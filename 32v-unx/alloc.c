/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/
#include "h/param.h"
#include "h/systm.h"
#include "h/mount.h"
#include "h/filsys.h"
#include "h/fblk.h"
#include "h/conf.h"
#include "h/buf.h"
#include "h/inode.h"
#include "h/ino.h"
#include "h/dir.h"
#include "h/user.h"
typedef struct fblk *FBLKP;
/*VC isn't picket this up from systm.h*/


/*
 * alloc will obtain the next available
 * free disk block from the free list of
 * the specified device.
 * The super block has up to NICFREE remembered
 * free blocks; the last of these is read to
 * obtain NICFREE more . . .
 *
 * no space on dev x/y -- when
 * the free list is exhausted.
 */
struct buf *
UNIX_alloc(dev)
dev_t dev;
{
        daddr_t bno;
        register struct filsys *fp;
        register struct buf *bp;

        fp = UNIX_getfs(dev);
        while(fp->s_flock)
                UNIX_sleep((caddr_t)&fp->s_flock, PINOD);
        do {
                if(fp->s_nfree <= 0)
                        goto nospace;
                if (fp->s_nfree > NICFREE) {
                        UNIX_prdev("Bad free count", dev);
                        goto nospace;
                }
                bno = fp->s_free[--fp->s_nfree];
                if(bno == 0)
                        goto nospace;
        } while (UNIX_badblock(fp, bno, dev));
        if(fp->s_nfree <= 0) {
                fp->s_flock++;
                bp = UNIX_bread(dev, bno);
                if ((bp->b_flags&B_ERROR) == 0) {
                        fp->s_nfree = ((FBLKP)(bp->b_un.b_addr))->df_nfree;
                        UNIX_bcopy((caddr_t)((FBLKP)(bp->b_un.b_addr))->df_free,
                            (caddr_t)fp->s_free, sizeof(fp->s_free));
                }
                UNIX_brelse(bp);
                fp->s_flock = 0;
                UNIX_wakeup((caddr_t)&fp->s_flock);
                if (fp->s_nfree <=0)
                        goto nospace;
        }
        bp = UNIX_getblk(dev, bno);
        UNIX_clrbuf(bp);
        fp->s_fmod = 1;
        return(bp);

nospace:
        fp->s_nfree = 0;
        UNIX_prdev("no space", dev);
        u.u_error = ENOSPC;
        return(NULL);
}

/*
 * place the specified disk block
 * back on the free list of the
 * specified device.
 */
UNIX_free(dev, bno)
dev_t dev;
daddr_t bno;
{
        register struct filsys *fp;
        register struct buf *bp;

        fp = UNIX_getfs(dev);
        fp->s_fmod = 1;
        while(fp->s_flock)
                UNIX_sleep((caddr_t)&fp->s_flock, PINOD);
        if (UNIX_badblock(fp, bno, dev))
                return;
        if(fp->s_nfree <= 0) {
                fp->s_nfree = 1;
                fp->s_free[0] = 0;
        }
        if(fp->s_nfree >= NICFREE) {
                fp->s_flock++;
                bp = UNIX_getblk(dev, bno);
                ((FBLKP)(bp->b_un.b_addr))->df_nfree = fp->s_nfree;
                UNIX_bcopy((caddr_t)fp->s_free,
                        (caddr_t)((FBLKP)(bp->b_un.b_addr))->df_free,
                        sizeof(fp->s_free));
                fp->s_nfree = 0;
                UNIX_bwrite(bp);
                fp->s_flock = 0;
                UNIX_wakeup((caddr_t)&fp->s_flock);
        }
        fp->s_free[fp->s_nfree++] = bno;
        fp->s_fmod = 1;
}

/*
 * Check that a block number is in the
 * range between the I list and the size
 * of the device.
 * This is used mainly to check that a
 * garbage file system has not been mounted.
 *
 * bad block on dev x/y -- not in range
 */
UNIX_badblock(fp, bn, dev)
register struct filsys *fp;
daddr_t bn;
dev_t dev;
{

        if (bn < fp->s_isize || bn >= fp->s_fsize) {
                UNIX_prdev("bad block", dev);
                return(1);
        }
        return(0);
}

/*
 * Allocate an unused I node
 * on the specified device.
 * Used with file creation.
 * The algorithm keeps up to
 * NICINOD spare I nodes in the
 * super block. When this runs out,
 * a linear search through the
 * I list is instituted to pick
 * up NICINOD more.
 */
struct inode *
UNIX_ialloc(dev)
dev_t dev;
{
        register struct filsys *fp;
        register struct buf *bp;
        register struct inode *ip;
        int i;
        struct dinode *dp;
        ino_t ino;
        daddr_t adr;

        fp = UNIX_getfs(dev);
        while(fp->s_ilock)
                UNIX_sleep((caddr_t)&fp->s_ilock, PINOD);
loop:
        if(fp->s_ninode > 0) {
                ino = fp->s_inode[--fp->s_ninode];
                ip = UNIX_iget(dev, ino);
                if(ip == NULL)
                        return(NULL);
                if(ip->i_mode == 0) {
                        for (i=0; i<NADDR; i++)
				{
                                ip->i_un.a.i_addr[i] = 0;
                                //ip->i_un.i_addr[i] = 0;
				}
                        fp->s_fmod = 1;
                        return(ip);
                }
                /*
                 * Inode was allocated after all.
                 * Look some more.
                 */
                UNIX_iput(ip);
                goto loop;
        }
        fp->s_ilock++;
        ino = 1;
        for(adr = SUPERB+1; adr < fp->s_isize; adr++) {
                bp = UNIX_bread(dev, adr);
                if (bp->b_flags & B_ERROR) {
                        UNIX_brelse(bp);
                        ino += INOPB;
                        continue;
                }
                dp = bp->b_un.b_dino;
                for(i=0; i<INOPB; i++) {
                        if(dp->di_mode != 0)
                                goto cont;
                        for(ip = &inode[0]; ip < &inode[NINODE]; ip++)
                        if(dev==ip->i_dev && ino==ip->i_number)
                                goto cont;
                        fp->s_inode[fp->s_ninode++] = ino;
                        if(fp->s_ninode >= NICINOD)
                                break;
                cont:
                        ino++;
                        dp++;
                }
                UNIX_brelse(bp);
                if(fp->s_ninode >= NICINOD)
                        break;
        }
        fp->s_ilock = 0;
        UNIX_wakeup((caddr_t)&fp->s_ilock);
        if(fp->s_ninode > 0)
                goto loop;
        UNIX_prdev("Out of inodes", dev);
        u.u_error = ENOSPC;
        return(NULL);
}

/*
 * Free the specified I node
 * on the specified device.
 * The algorithm stores up
 * to NICINOD I nodes in the super
 * block and throws away any more.
 */
UNIX_ifree(dev, ino)
dev_t dev;
ino_t ino;
{
        register struct filsys *fp;

        fp = UNIX_getfs(dev);
        if(fp->s_ilock)
                return;
        if(fp->s_ninode >= NICINOD)
                return;
        fp->s_inode[fp->s_ninode++] = ino;
        fp->s_fmod = 1;
}

/*
 * getfs maps a device number into
 * a pointer to the incore super
 * block.
 * The algorithm is a linear
 * search through the mount table.
 * A consistency check of the
 * in core free-block and i-node
 * counts.
 *
 * bad count on dev x/y -- the count
 *      check failed. At this point, all
 *      the counts are zeroed which will
 *      almost certainly lead to "no space"
 *      diagnostic
 * UNIX_panic: no fs -- the device is not mounted.
 *      this "cannot happen"
 */
struct filsys *
UNIX_getfs(dev)
dev_t dev;
{
        register struct mount *mp;
        register struct filsys *fp;

        for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
        if(mp->m_bufp != NULL && mp->m_dev == dev) {
                fp = mp->m_bufp->b_un.b_filsys;
                if(fp->s_nfree > NICFREE || fp->s_ninode > NICINOD) {
                        UNIX_prdev("bad count", dev);
                        fp->s_nfree = 0;
                        fp->s_ninode = 0;
                }
                return(fp);
        }
        UNIX_panic("no fs");
        return(NULL);
}

/*
 * update is the internal name of
 * 'sync'. It goes through the disk
 * queues to initiate sandbagged IO;
 * goes through the I nodes to write
 * modified nodes; and it goes through
 * the mount table to initiate modified
 * super blocks.
 */
UNIX_update()
{
        register struct inode *ip;
        register struct mount *mp;
        register struct buf *bp;
        struct filsys *fp;

        if(updlock)
                return;
        updlock++;
        for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
                if(mp->m_bufp != NULL) {
                        fp = mp->m_bufp->b_un.b_filsys;
                        if(fp->s_fmod==0 || fp->s_ilock!=0 ||
                           fp->s_flock!=0 || fp->s_ronly!=0)
                                continue;
                        bp = UNIX_getblk(mp->m_dev, SUPERB);
                        if (bp->b_flags & B_ERROR)
                                continue;
                        fp->s_fmod = 0;
                        fp->s_time = time;
                        UNIX_bcopy((caddr_t)fp, bp->b_un.b_addr, BSIZE);
                        UNIX_bwrite(bp);
                }
        for(ip = &inode[0]; ip < &inode[NINODE]; ip++)
                if((ip->i_flag&ILOCK)==0 && ip->i_count) {
                        ip->i_flag |= ILOCK;
                        ip->i_count++;
                        UNIX_iupdat(ip, &time, &time);
                        UNIX_iput(ip);
                }
        updlock = 0;
        UNIX_bflush(NODEV);
}
