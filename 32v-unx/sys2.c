/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/reg.h"
#include "h/file.h"
#include "h/inode.h"

/*
 * read system call
 */
UNIX_read()
{
        UNIX_rdwr(FREAD);
}

/*
 * write system call
 */
UNIX_write()
{
        UNIX_rdwr(FWRITE);
}

/*
 * common code for read and write calls:
 * check permissions, set base, count, and offset,
 * and switch out to readi, writei, or pipe code.
 */
UNIX_rdwr(mode)
register mode;
{
        register struct file *fp;
        register struct inode *ip;
        register struct a {
                int     fdes;
                char    *cbuf;
                unsigned count;
        } *uap;

        uap = (struct a *)u.u_ap;
        fp = UNIX_getf(uap->fdes);
        if(fp == NULL)
                return;
        if((fp->f_flag&mode) == 0) {
                u.u_error = EBADF;
                return;
        }
        u.u_base = (caddr_t)uap->cbuf;
        u.u_count = uap->count;
        u.u_segflg = 0;
        if((fp->f_flag&FPIPE) != 0) {
                if(mode == FREAD)
                        UNIX_readp(fp);
                else
                        UNIX_writep(fp);
        } else {
                ip = fp->f_inode;
                if (fp->f_flag&FMP)
                        u.u_offset = 0;
                else
                        u.u_offset = fp->f_un.f_offset;
                if((ip->i_mode&(IFCHR&IFBLK)) == 0)
                        UNIX_plock(ip);
                if(mode == FREAD)
                        UNIX_readi(ip);
                else
                        UNIX_writei(ip);
                if((ip->i_mode&(IFCHR&IFBLK)) == 0)
                        UNIX_prele(ip);
                if ((fp->f_flag&FMP) == 0)
                        fp->f_un.f_offset += uap->count-u.u_count;
        }
        u.u_r.a.r_val1 = uap->count-u.u_count;
}

/*
 * open system call
 */
UNIX_open()
{
        register struct inode *ip;
        register struct a {
                char    *fname;
                int     rwmode;
        } *uap;

        uap = (struct a *)u.u_ap;
        ip = UNIX_namei(UNIX_uchar, 0);
        if(ip == NULL)
                return;
        UNIX_open1(ip, ++uap->rwmode, 0);
}

/*
 * creat system call
 */
UNIX_creat()
{
        register struct inode *ip;
        register struct a {
                char    *fname;
                int     fmode;
        } *uap;

        uap = (struct a *)u.u_ap;
        ip = UNIX_namei(UNIX_uchar, 1);
        if(ip == NULL) {
                if(u.u_error)
                        return;
                ip = UNIX_maknode(uap->fmode&07777&(~ISVTX));
                if (ip==NULL)
                        return;
                UNIX_open1(ip, FWRITE, 2);
        } else
                UNIX_open1(ip, FWRITE, 1);
}

/*
 * common code for open and creat.
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
UNIX_open1(ip, mode, trf)
register struct inode *ip;
register mode;
{
        register struct file *fp;
        int i;

        if(trf != 2) {
                if(mode&FREAD)
                        UNIX_access(ip, IREAD);
                if(mode&FWRITE) {
                        UNIX_access(ip, IWRITE);
                        if((ip->i_mode&IFMT) == IFDIR)
                                u.u_error = EISDIR;
                }
        }
        if(u.u_error)
                goto out;
        if(trf == 1)
                UNIX_itrunc(ip);
        UNIX_prele(ip);
        if ((fp = UNIX_falloc()) == NULL)
                goto out;
        fp->f_flag = mode&(FREAD|FWRITE);
        fp->f_inode = ip;
        i = u.u_r.a.r_val1;
        UNIX_openi(ip, mode&FWRITE);
        if(u.u_error == 0)
                return;
        u.u_ofile[i] = NULL;
        fp->f_count--;

out:
        UNIX_iput(ip);
}

/*
 * close system call
 */
UNIX_close()
{
        register struct file *fp;
        register struct a {
                int     fdes;
        } *uap;

        uap = (struct a *)u.u_ap;
        fp = UNIX_getf(uap->fdes);
        if(fp == NULL)
                return;
        u.u_ofile[uap->fdes] = NULL;
        UNIX_closef(fp);
}

/*
 * seek system call
 */
UNIX_seek()
{
        register struct file *fp;
        register struct a {
                int     fdes;
                U_off_t   off;
                int     sbase;
        } *uap;

        uap = (struct a *)u.u_ap;
        fp = UNIX_getf(uap->fdes);
        if(fp == NULL)
                return;
        if(fp->f_flag&(FPIPE|FMP)) {
                u.u_error = ESPIPE;
                return;
        }
        if(uap->sbase == 1)
                uap->off += fp->f_un.f_offset;
        else if(uap->sbase == 2)
                uap->off += fp->f_inode->i_size;
        fp->f_un.f_offset = uap->off;
        u.u_r.r_off = uap->off;
}


/*
 * link system call
 */
UNIX_link()
{
        register struct inode *ip, *xp;
        register struct a {
                char    *target;
                char    *linkname;
        } *uap;

        uap = (struct a *)u.u_ap;
        ip = UNIX_namei(UNIX_uchar, 0);
        if(ip == NULL)
                return;
        if((ip->i_mode&IFMT)==IFDIR && !UNIX_suser())
                goto out;
        /*
         * Unlock to avoid possibly hanging the namei.
         * Sadly, this means races. (Suppose someone
         * deletes the file in the meantime?)
         * Nor can it be locked again later
         * because then there will be deadly
         * embraces.
         */
        UNIX_prele(ip);
        u.u_dirp = (caddr_t)uap->linkname;
        xp = UNIX_namei(UNIX_uchar, 1);
        if(xp != NULL) {
                u.u_error = EEXIST;
                UNIX_iput(xp);
                goto out;
        }
        if (u.u_error)
                goto out;
        if(u.u_pdir->i_dev != ip->i_dev) {
                UNIX_iput(u.u_pdir);
                u.u_error = EXDEV;
                goto out;
        }
        UNIX_wdir(ip);
        if (u.u_error==0) {
                ip->i_nlink++;
                ip->i_flag |= ICHG;
        }

out:
        UNIX_iput(ip);
}

/*
 * mknod system call
 */
UNIX_mknod()
{
        register struct inode *ip;
        register struct a {
                char    *fname;
                int     fmode;
                int     dev;
        } *uap;

        uap = (struct a *)u.u_ap;
        if(UNIX_suser()) {
                ip = UNIX_namei(UNIX_uchar, 1);
                if(ip != NULL) {
                        u.u_error = EEXIST;
                        goto out;
                }
        }
        if(u.u_error)
                return;
        ip = UNIX_maknode(uap->fmode);
        if (ip == NULL)
                return;
        ip->i_un.b.i_rdev = (dev_t)uap->dev;

out:
        UNIX_iput(ip);
}

/*
 * access system call
 */
UNIX_saccess()
{
        register svuid, svgid;
        register struct inode *ip;
        register struct a {
                char    *fname;
                int     fmode;
        } *uap;

        uap = (struct a *)u.u_ap;
        svuid = u.u_uid;
        svgid = u.u_gid;
        u.u_uid = u.u_ruid;
        u.u_gid = u.u_rgid;
        ip = UNIX_namei(UNIX_uchar, 0);
        if (ip != NULL) {
                if (uap->fmode&(IREAD>>6))
                        UNIX_access(ip, IREAD);
                if (uap->fmode&(IWRITE>>6))
                        UNIX_access(ip, IWRITE);
                if (uap->fmode&(IEXEC>>6))
                        UNIX_access(ip, IEXEC);
                UNIX_iput(ip);
        }
        u.u_uid = svuid;
        u.u_gid = svgid;
}
