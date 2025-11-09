/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/mount.h"
#include "h/ino.h"
#include "h/reg.h"
#include "h/buf.h"
#include "h/filsys.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/inode.h"
#include "h/file.h"
#include "h/conf.h"
#include "h/stat.h"

/*
 * the fstat system call.
 */
UNIX_fstat()
{
        register struct file *fp;
        register struct a {
                int     fdes;
                struct stat *sb;
        } *uap;

        uap = (struct a *)u.u_ap;
        fp = UNIX_getf(uap->fdes);
        if(fp == NULL)
                return;
        UNIX_stat1(fp->f_inode, uap->sb, fp->f_flag&FPIPE? fp->f_un.f_offset: 0);
}

/*
 * the stat system call.
 */
UNIX_stat()
{
        register struct inode *ip;
        register struct a {
                char    *fname;
                struct stat *sb;
        } *uap;

        uap = (struct a *)u.u_ap;
        ip = UNIX_namei(UNIX_uchar, 0);
        if(ip == NULL)
                return;
        UNIX_stat1(ip, uap->sb, (off_t)0);
        UNIX_iput(ip);
}

/*
 * The basic routine for fstat and stat:
 * get the inode and pass appropriate parts back.
 */
UNIX_stat1(ip, ub, pipeadj)
register struct inode *ip;
struct stat *ub;
off_t pipeadj;
{
        register struct dinode *dp;
        register struct buf *bp;
        struct stat ds;

        UNIX_iupdat(ip, &time, &time);
        /*
         * first copy from inode table
         */
        ds.st_dev = ip->i_dev;
        ds.st_ino = ip->i_number;
        ds.st_mode = ip->i_mode;
        ds.st_nlink = ip->i_nlink;
        ds.st_uid = ip->i_uid;
        ds.st_gid = ip->i_gid;
        ds.st_rdev = (dev_t)ip->i_un.b.i_rdev;
        ds.st_size = ip->i_size - pipeadj;
        /*
         * next the dates in the disk
         */
        bp = UNIX_bread(ip->i_dev, itod(ip->i_number));
        dp = bp->b_un.b_dino;
        dp += itoo(ip->i_number);
        ds.st_atime = dp->di_atime;
        ds.st_mtime = dp->di_mtime;
        ds.st_ctime = dp->di_ctime;
        UNIX_brelse(bp);
        if (UNIX_copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
                u.u_error = EFAULT;
}

/*
 * the dup system call.
 */
UNIX_dup()
{
        register struct file *fp;
        register struct a {
                int     fdes;
                int     fdes2;
        } *uap;
        register i, m;

        uap = (struct a *)u.u_ap;
        m = uap->fdes & ~077;
        uap->fdes &= 077;
        fp = UNIX_getf(uap->fdes);
        if(fp == NULL)
                return;
        if ((m&0100) == 0) {
                if ((i = UNIX_ufalloc()) < 0)
                        return;
        } else {
                i = uap->fdes2;
                if (i<0 || i>=NOFILE) {
                        u.u_error = EBADF;
                        return;
                }
                u.u_r.a.r_val1 = i;
        }
        if (i!=uap->fdes) {
                if (u.u_ofile[i]!=NULL)
                        UNIX_closef(u.u_ofile[i]);
                u.u_ofile[i] = fp;
                fp->f_count++;
        }
}

/*
 * the mount system call.
 */
UNIX_smount()
{
        dev_t dev;
        register struct inode *ip;
        register struct mount *mp;
        struct mount *smp;
        register struct filsys *fp;
        struct buf *bp;
        register struct a {
                char    *fspec;
                char    *freg;
                int     ronly;
        } *uap;

        uap = (struct a *)u.u_ap;
        dev = UNIX_getmdev();
        if(u.u_error)
                return;
        u.u_dirp = (caddr_t)uap->freg;
        ip = UNIX_namei(UNIX_uchar, 0);
        if(ip == NULL)
                return;
        if(ip->i_count!=1 || (ip->i_mode&(IFBLK&IFCHR))!=0)
                goto out;
        smp = NULL;
        for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
                if(mp->m_bufp != NULL) {
                        if(dev == mp->m_dev)
                                goto out;
                } else
                if(smp == NULL)
                        smp = mp;
        }
        mp = smp;
        if(mp == NULL)
                goto out;
        (*bdevsw[major(dev)].d_open)(dev, !uap->ronly);
        if(u.u_error)
                goto out;
        bp = UNIX_bread(dev, SUPERB);
        if(u.u_error) {
                UNIX_brelse(bp);
                goto out1;
        }
        mp->m_inodp = ip;
        mp->m_dev = dev;
        mp->m_bufp = UNIX_geteblk();
        UNIX_bcopy((caddr_t)bp->b_un.b_addr, mp->m_bufp->b_un.b_addr, BSIZE);
        fp = mp->m_bufp->b_un.b_filsys;
        fp->s_ilock = 0;
        fp->s_flock = 0;
        fp->s_ronly = uap->ronly & 1;
        UNIX_brelse(bp);
        ip->i_flag |= IMOUNT;
        UNIX_prele(ip);
        return;

out:
        u.u_error = EBUSY;
out1:
        UNIX_iput(ip);
}

/*
 * the umount system call.
 */
UNIX_sumount()
{
        dev_t dev;
        register struct inode *ip;
        register struct mount *mp;
        struct buf *bp;
        register struct a {
                char    *fspec;
        };

        dev = UNIX_getmdev();
        if(u.u_error)
                return;
        UNIX_xumount(dev);      /* remove unused sticky files from text table */
        UNIX_update();
        for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
                if(mp->m_bufp != NULL && dev == mp->m_dev)
                        goto found;
        u.u_error = EINVAL;
        return;

found:
        for(ip = &inode[0]; ip < &inode[NINODE]; ip++)
                if(ip->i_number != 0 && dev == ip->i_dev) {
                        u.u_error = EBUSY;
                        return;
                }
        (*bdevsw[major(dev)].d_close)(dev, 0);
        ip = mp->m_inodp;
        ip->i_flag &= ~IMOUNT;
        UNIX_plock(ip);
        UNIX_iput(ip);
        bp = mp->m_bufp;
        mp->m_bufp = NULL;
        UNIX_brelse(bp);
}

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
dev_t
UNIX_getmdev()
{
        dev_t dev;
        register struct inode *ip;

        ip = UNIX_namei(UNIX_uchar, 0);
        if(ip == NULL)
                return(NODEV);
        if((ip->i_mode&IFMT) != IFBLK)
                u.u_error = ENOTBLK;
        dev = (dev_t)ip->i_un.b.i_rdev;
        if(major(dev) >= nblkdev)
                u.u_error = ENXIO;
        UNIX_iput(ip);
        return(dev);
}
