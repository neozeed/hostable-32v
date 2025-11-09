/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/inode.h"
#include "h/file.h"
#include "h/reg.h"

/*
 * Max allowable buffering per pipe.
 * This is also the max size of the
 * file created to implement the pipe.
 * If this size is bigger than 5120,
 * pipes will be implemented with large
 * files, which is probably not good.
 */
#define PIPSIZ  4096

/*
 * The sys-pipe entry.
 * Allocate an inode on the root device.
 * Allocate 2 file structures.
 * Put it all together with flags.
 */
UNIX_pipe()
{
        register struct inode *ip;
        register struct file *rf, *wf;
        int r;

        ip = UNIX_ialloc(pipedev);
        if(ip == NULL)
                return;
        rf = UNIX_falloc();
        if(rf == NULL) {
                UNIX_iput(ip);
                return;
        }
        //r = u.u_r.r_val1;
        r = u.u_r.a.r_val1;
        wf = UNIX_falloc();
        if(wf == NULL) {
                rf->f_count = 0;
                u.u_ofile[r] = NULL;
                UNIX_iput(ip);
                return;
        }
        //u.u_r.r_val2 = u.u_r.r_val1;
        u.u_r.a.r_val2 = u.u_r.a.r_val1;
        //u.u_r.r_val1 = r;
        u.u_r.a.r_val1 = r;
        wf->f_flag = FWRITE|FPIPE;
        wf->f_inode = ip;
        rf->f_flag = FREAD|FPIPE;
        rf->f_inode = ip;
        ip->i_count = 2;
        ip->i_mode = IFREG;
        ip->i_flag = IACC|IUPD|ICHG;
}

/*
 * Read call directed to a pipe.
 */
UNIX_readp(fp)
register struct file *fp;
{
        register struct inode *ip;

        ip = fp->f_inode;

loop:
        /*
         * Very conservative locking.
         */

        UNIX_plock(ip);
        /*
         * If nothing in the pipe, wait.
         */
        if (ip->i_size == 0) {
                /*
                 * If there are not both reader and
                 * writer active, return without
                 * satisfying read.
                 */
                UNIX_prele(ip);
                if(ip->i_count < 2)
                        return;
                ip->i_mode |= IREAD;
                UNIX_sleep((caddr_t)ip+2, PPIPE);
                goto loop;
        }

        /*
         * Read and return
         */

        u.u_offset = fp->f_un.f_offset;
        UNIX_readi(ip);
        fp->f_un.f_offset = u.u_offset;
        /*
         * If reader has caught up with writer, reset
         * offset and size to 0.
         */
        if (fp->f_un.f_offset == ip->i_size) {
                fp->f_un.f_offset = 0;
                ip->i_size = 0;
                if(ip->i_mode & IWRITE) {
                        ip->i_mode &= ~IWRITE;
                        UNIX_wakeup((caddr_t)ip+1);
                }
        }
        UNIX_prele(ip);
}

/*
 * Write call directed to a pipe.
 */
UNIX_writep(fp)
register struct file *fp;
{
        register c;
        register struct inode *ip;

        ip = fp->f_inode;
        c = u.u_count;

loop:

        /*
         * If all done, return.
         */

        UNIX_plock(ip);
        if(c == 0) {
                UNIX_prele(ip);
                u.u_count = 0;
                return;
        }

        /*
         * If there are not both read and
         * write sides of the pipe active,
         * return error and signal too.
         */

        if(ip->i_count < 2) {
                UNIX_prele(ip);
                u.u_error = EPIPE;
                UNIX_psignal(u.u_procp, SIGPIPE);
                return;
        }

        /*
         * If the pipe is full,
         * wait for reads to deplete
         * and truncate it.
         */

        if(ip->i_size >= PIPSIZ) {
                ip->i_mode |= IWRITE;
                UNIX_prele(ip);
                UNIX_sleep((caddr_t)ip+1, PPIPE);
                goto loop;
        }

        /*
         * Write what is possible and
         * loop back.
         * If writing less than PIPSIZ, it always goes.
         * One can therefore get a file > PIPSIZ if write
         * sizes do not divide PIPSIZ.
         */

        u.u_offset = ip->i_size;
        u.u_count = UNIX_min((unsigned)c, (unsigned)PIPSIZ);
        c -= u.u_count;
        UNIX_writei(ip);
        UNIX_prele(ip);
        if(ip->i_mode&IREAD) {
                ip->i_mode &= ~IREAD;
                UNIX_wakeup((caddr_t)ip+2);
        }
        goto loop;
}

/*
 * Lock a pipe.
 * If its already locked,
 * set the WANT bit and sleep.
 */
UNIX_plock(ip)
register struct inode *ip;
{

        while(ip->i_flag&ILOCK) {
                ip->i_flag |= IWANT;
                UNIX_sleep((caddr_t)ip, PINOD);
        }
        ip->i_flag |= ILOCK;
}

/*
 * Unlock a pipe.
 * If WANT bit is on,
 * wakeup.
 * This routine is also used
 * to unlock inodes in general.
 */
UNIX_prele(ip)
register struct inode *ip;
{

        ip->i_flag &= ~ILOCK;
        if(ip->i_flag&IWANT) {
                ip->i_flag &= ~IWANT;
                UNIX_wakeup((caddr_t)ip);
        }
}
