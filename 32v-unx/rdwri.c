/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/inode.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/buf.h"
#include "h/conf.h"

/*
 * Read the file corresponding to
 * the inode pointed at by the argument.
 * The actual read arguments are found
 * in the variables:
 *      u_base          core address for destination
 *      u_offset        byte offset in file
 *      u_count         number of bytes to read
 *      u_segflg        read to kernel/user/user I
 */
UNIX_readi(ip)
register struct inode *ip;
{
        struct buf *bp;
        dev_t dev;
        daddr_t lbn, bn;
        off_t diff;
        register on, n;
        register type;
        extern int mem_no;

        if(u.u_count == 0)
                return(0);
        dev = (dev_t)ip->i_un.b.i_rdev;
        if(u.u_offset < 0 && ((ip->i_mode&IFMT) != IFCHR || mem_no != major(dev)) ) {
                u.u_error = EINVAL;
                return(0);
        }
        ip->i_flag |= IACC;
        type = ip->i_mode&IFMT;
        if (type==IFCHR || type==IFMPC) {
                return((*cdevsw[major(dev)].d_read)(dev));
        }

        do {
                lbn = bn = u.u_offset >> BSHIFT;
                on = u.u_offset & BMASK;
                n = UNIX_min((unsigned)(BSIZE-on), u.u_count);
                if (type!=IFBLK && type!=IFMPB) {
                        diff = ip->i_size - u.u_offset;
                        if(diff <= 0)
                                return(0);
                        if(diff < n)
                                n = diff;
                        bn = UNIX_bmap(ip, bn, B_READ);
                        if(u.u_error)
                                return(0);
                        dev = ip->i_dev;
                } else
                        rablock = bn+1;
                if ((long)bn<0) {
                        bp = UNIX_geteblk();
                        UNIX_clrbuf(bp);
                } else if (ip->i_un.a.i_lastr+1==lbn)
                        bp = UNIX_breada(dev, bn, rablock);
                else
                        bp = UNIX_bread(dev, bn);
                ip->i_un.a.i_lastr = lbn;
                n = UNIX_min((unsigned)n, BSIZE-bp->b_resid);
                if (n!=0)
                        UNIX_iomove(bp->b_un.b_addr+on, n, B_READ);
                UNIX_brelse(bp);
        } while(u.u_error==0 && u.u_count!=0 && n>0);
}

/*
 * Write the file corresponding to
 * the inode pointed at by the argument.
 * The actual write arguments are found
 * in the variables:
 *      u_base          core address for source
 *      u_offset        byte offset in file
 *      u_count         number of bytes to write
 *      u_segflg        write to kernel/user/user I
 */
UNIX_writei(ip)
register struct inode *ip;
{
        struct buf *bp;
        dev_t dev;
        daddr_t bn;
        register n, on;
        register type;
        extern int mem_no;

        dev = (dev_t)ip->i_un.b.i_rdev;
        if(u.u_offset < 0 && ((ip->i_mode&IFMT) != IFCHR || mem_no != major(dev)) ) {
                u.u_error = EINVAL;
                return;
        }
        type = ip->i_mode&IFMT;
        if (type==IFCHR || type==IFMPC) {
                ip->i_flag |= IUPD|ICHG;
                (*cdevsw[major(dev)].d_write)(dev);
                return;
        }
        if (u.u_count == 0)
                return;

        do {
                bn = u.u_offset >> BSHIFT;
                on = u.u_offset & BMASK;
                n = UNIX_min((unsigned)(BSIZE-on), u.u_count);
                if (type!=IFBLK && type!=IFMPB) {
                        bn = UNIX_bmap(ip, bn, B_WRITE);
                        if((long)bn<0)
                                return;
                        dev = ip->i_dev;
                }
                if(n == BSIZE) 
                        bp = UNIX_getblk(dev, bn);
                else
                        bp = UNIX_bread(dev, bn);
                UNIX_iomove(bp->b_un.b_addr+on, n, B_WRITE);
                if(u.u_error != 0)
                        UNIX_brelse(bp);
                else
                        UNIX_bdwrite(bp);
                if(u.u_offset > ip->i_size &&
                   (type==IFDIR || type==IFREG))
                        ip->i_size = u.u_offset;
                ip->i_flag |= IUPD|ICHG;
        } while(u.u_error==0 && u.u_count!=0);
}

/*
 * Return the logical maximum
 * of the 2 arguments.
 */
UNIX_max(a, b)
unsigned a, b;
{

        if(a > b)
                return(a);
        return(b);
}

/*
 * Return the logical minimum
 * of the 2 arguments.
 */
UNIX_min(a, b)
unsigned a, b;
{

        if(a < b)
                return(a);
        return(b);
}

/*
 * Move n bytes at byte location
 * &bp->b_un.b_addr[o] to/from (flag) the
 * user/kernel (u.segflg) area starting at u.base.
 * Update all the arguments by the number
 * of bytes moved.
 */
UNIX_iomove(cp, n, flag)
register caddr_t cp;
register n;
{
        register t;

        if (n==0)
                return;
        if(u.u_segflg != 1)  {
                if (flag==B_WRITE)
                        t = UNIX_copyin(u.u_base, (caddr_t)cp, n);
                else
                        t = UNIX_copyout((caddr_t)cp, u.u_base, n);
                if (t) {
                        u.u_error = EFAULT;
                        return;
                }
        }
        else
                if (flag == B_WRITE)
                        UNIX_bcopy(u.u_base,(caddr_t)cp,n);
                else
                        UNIX_bcopy((caddr_t)cp,u.u_base,n);
        u.u_base += n;
        u.u_offset += n;
        u.u_count -= n;
        return;
}
