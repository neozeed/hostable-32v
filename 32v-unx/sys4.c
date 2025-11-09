/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/reg.h"
#include "h/inode.h"
#include "h/proc.h"
#include "h/clock.h"
#include "h/mtpr.h"
#include "h/timeb.h"

/*
 * Everything in this file is a routine implementing a system call.
 */

/*
 * return the current time (old-style entry)
 */
UNIX_gtime()
{
        u.u_r.r_time = time;
}

/*
 * New time entry-- return TOD with milliseconds, timezone,
 * DST flag
 */
UNIX_ftime()
{
        register struct a {
                struct  timeb   *tp;
        } *uap;
        struct timeb t;
        register unsigned ms;

        uap = (struct a *)u.u_ap;
        spl7();
        t.time = time;
        ms = lbolt;
        spl0();
        if (ms > HZ) {
                ms -= HZ;
                t.time++;
        }
        t.millitm = (1000*ms)/HZ;
        t.timezone = TIMEZONE;
        t.dstflag = DSTFLAG;
        if (UNIX_copyout((caddr_t)&t, (caddr_t)uap->tp, sizeof(t)) < 0)
                u.u_error = EFAULT;
}

/*
 * Set the time
 */
UNIX_stime()
{
        register unsigned int i , j ;
        register struct a {
                time_t  time;
        } *uap;

        uap = (struct a *)u.u_ap;
        if(UNIX_suser()) {
                time = uap->time;
                /* In addition to setting software time (GMT seconds
                *  since YRREF), also set VAX TODR reg (10 ms. clicks
                *  into current year YRCURR
                */
                for (i = time , j = YRREF ; j < YRCURR ; j++)
                        i -= (SECYR + (j%4?0:SECDAY)) ;
                UNIX_mtpr(TODR,i*100) ; /* 10 ms. clicks */
        }
}

UNIX_setuid()
{
        register uid;
        register struct a {
                int     uid;
        } *uap;

        uap = (struct a *)u.u_ap;
        uid = uap->uid;
        if(u.u_ruid == uid || UNIX_suser()) {
                u.u_uid = uid;
                u.u_procp->p_uid = uid;
                u.u_ruid = uid;
        }
}

UNIX_getuid()
{

        u.u_r.a.r_val1 = u.u_ruid;
        u.u_r.a.r_val2 = u.u_uid;
}

UNIX_setgid()
{
        register gid;
        register struct a {
                int     gid;
        } *uap;

        uap = (struct a *)u.u_ap;
        gid = uap->gid;
        if(u.u_rgid == gid || UNIX_suser()) {
                u.u_gid = gid;
                u.u_rgid = gid;
        }
}

UNIX_getgid()
{

        u.u_r.a.r_val1 = u.u_rgid;
        u.u_r.a.r_val2 = u.u_gid;
}

UNIX_getpid()
{
        u.u_r.a.r_val1 = u.u_procp->p_pid;
        u.u_r.a.r_val2 = u.u_procp->p_ppid;
}

UNIX_sync()
{

        UNIX_update();
}

UNIX_nice()
{
        register n;
        register struct a {
                int     niceness;
        } *uap;

        uap = (struct a *)u.u_ap;
        n = uap->niceness;
        if(n < 0 && !UNIX_suser())
                n = 0;
        n += u.u_procp->p_nice;
        if(n >= 2*NZERO)
                n = 2*NZERO -1;
        if(n < 0)
                n = 0;
        u.u_procp->p_nice = n;
}

/*
 * Unlink system call.
 * Hard to avoid races here, especially
 * in unlinking directories.
 */
UNIX_unlink()
{
        register struct inode *ip, *pp;
        struct a {
                char    *fname;
        };

        pp = UNIX_namei(UNIX_uchar, 2);
        if(pp == NULL)
                return;
        /*
         * Check for unlink(".")
         * to avoid hanging on the iget
         */
        if (pp->i_number == u.u_dent.d_ino) {
                ip = pp;
                ip->i_count++;
        } else
                ip = UNIX_iget(pp->i_dev, u.u_dent.d_ino);
        if(ip == NULL)
                goto out1;
        if((ip->i_mode&IFMT)==IFDIR && !UNIX_suser())
                goto out;
        /*
         * Don't unlink a mounted file.
         */
        if (ip->i_dev != pp->i_dev) {
                u.u_error = EBUSY;
                goto out;
        }
        if (ip->i_flag&ITEXT)
                UNIX_xrele(ip); /* try once to free text */
        if (ip->i_flag&ITEXT && ip->i_nlink==1) {
                u.u_error = ETXTBSY;
                goto out;
        }
        u.u_offset -= sizeof(struct direct);
        u.u_base = (caddr_t)&u.u_dent;
        u.u_count = sizeof(struct direct);
        u.u_dent.d_ino = 0;
        UNIX_writei(pp);
        ip->i_nlink--;
        ip->i_flag |= ICHG;

out:
        UNIX_iput(ip);
out1:
        UNIX_iput(pp);
}
UNIX_chdir()
{
        chdirec(&u.u_cdir);
}

UNIX_chroot()
{
        chdirec(&u.u_rdir);
}

chdirec(ipp)
register struct inode **ipp;
{
        register struct inode *ip;
        struct a {
                char    *fname;
        };

        ip = UNIX_namei(UNIX_uchar, 0);
        if(ip == NULL)
                return;
        if((ip->i_mode&IFMT) != IFDIR) {
                u.u_error = ENOTDIR;
                goto bad;
        }
        if(UNIX_access(ip, IEXEC))
                goto bad;
        UNIX_prele(ip);
        if (*ipp) {
                UNIX_plock(*ipp);
                UNIX_iput(*ipp);
        }
        *ipp = ip;
        return;

bad:
        UNIX_iput(ip);
}

UNIX_chmod()
{
        register struct inode *ip;
        register struct a {
                char    *fname;
                int     fmode;
        } *uap;

        uap = (struct a *)u.u_ap;
        if ((ip = UNIX_owner()) == NULL)
                return;
        ip->i_mode &= ~07777;
        if (u.u_uid)
                uap->fmode &= ~ISVTX;
        ip->i_mode |= uap->fmode&07777;
        ip->i_flag |= ICHG;
        if (ip->i_flag&ITEXT && (ip->i_mode&ISVTX)==0)
                UNIX_xrele(ip);
        UNIX_iput(ip);
}

UNIX_chown()
{
        register struct inode *ip;
        register struct a {
                char    *fname;
                int     uid;
                int     gid;
        } *uap;

        uap = (struct a *)u.u_ap;
        if (!UNIX_suser() || (ip = UNIX_owner()) == NULL)
                return;
        ip->i_uid = uap->uid;
        ip->i_gid = uap->gid;
        ip->i_flag |= ICHG;
        UNIX_iput(ip);
}

UNIX_ssig()
{
        register a;
        struct a {
                int     signo;
                int     fun;
        } *uap;

        uap = (struct a *)u.u_ap;
        a = uap->signo;
        if(a<=0 || a>=NSIG || a==SIGKIL) {
                u.u_error = EINVAL;
                return;
        }
        u.u_r.a.r_val1 = u.u_signal[a];
        u.u_signal[a] = uap->fun;
        u.u_procp->p_sig &= ~(1<<(a-1));
}

UNIX_kill()
{
        register struct proc *p, *q;
        register a;
        register struct a {
                int     pid;
                int     signo;
        } *uap;
        int f, priv;

        uap = (struct a *)u.u_ap;
        f = 0;
        a = uap->pid;
        priv = 0;
        if (a==-1 && u.u_uid==0) {
                priv++;
                a = 0;
        }
        q = u.u_procp;
        for(p = &proc[0]; p < &proc[NPROC]; p++) {
                if(p->p_stat == NULL)
                        continue;
                if(a != 0 && p->p_pid != a)
                        continue;
                if(a==0 && ((p->p_pgrp!=q->p_pgrp&&priv==0) || p<=&proc[1]))
                        continue;
                if(u.u_uid != 0 && u.u_uid != p->p_uid)
                        continue;
                f++;
                UNIX_psignal(p, uap->signo);
        }
        if(f == 0)
                u.u_error = ESRCH;
}

UNIX_times()
{
        register struct a {
                time_t  (*times)[4];
        } *uap;

        uap = (struct a *)u.u_ap;
        if (UNIX_copyout((caddr_t)&u.u_utime, (caddr_t)uap->times, sizeof(*uap->times)) < 0)
                u.u_error = EFAULT;
}

UNIX_profil()
{
        register struct a {
                short   *bufbase;
                unsigned bufsize;
                unsigned pcoffset;
                unsigned pcscale;
        } *uap;

        uap = (struct a *)u.u_ap;
        u.u_prof.pr_base = uap->bufbase;
        u.u_prof.pr_size = uap->bufsize;
        u.u_prof.pr_off = uap->pcoffset;
        u.u_prof.pr_scale = uap->pcscale;
}

/*
 * alarm clock signal
 */
UNIX_alarm()
{
        register struct proc *p;
        register c;
        register struct a {
                int     deltat;
        } *uap;

        uap = (struct a *)u.u_ap;
        p = u.u_procp;
        c = p->p_clktim;
        p->p_clktim = uap->deltat;
        u.u_r.a.r_val1 = c;
}

/*
 * indefinite wait.
 * no one should wakeup(&u)
 */
UNIX_pause()
{

        for(;;)
                UNIX_sleep((caddr_t)&u, PSLEP);
}

/*
 * mode mask for creation of files
 */
UNIX_umask()
{
        register struct a {
                int     mask;
        } *uap;
        register t;

        uap = (struct a *)u.u_ap;
        t = u.u_cmask;
        u.u_cmask = uap->mask & 0777;
        u.u_r.a.r_val1 = t;
}

/*
 * Set IUPD and IACC times on file.
 * Can't set ICHG.
 */
UNIX_utime()
{
        register struct a {
                char    *fname;
                time_t  *tptr;
        } *uap;
        register struct inode *ip;
        time_t tv[2];

        uap = (struct a *)u.u_ap;
        if ((ip = UNIX_owner()) == NULL)
                return;
        if (UNIX_copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof(tv))) {
                u.u_error = EFAULT;
                return;
        }
        ip->i_flag |= IACC|IUPD|ICHG;
        UNIX_iupdat(ip, &tv[0], &tv[1]);
        UNIX_iput(ip);
}
