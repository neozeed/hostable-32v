/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/reg.h"
#include "h/proc.h"
#include "h/tty.h"
#include "h/inode.h"
#define KERNEL  1
#include "h/mx.h"
#include "h/file.h"
#include "h/conf.h"

/*
 * Multiplexor:   clist version
 *
 * installation:
 *      requires a line in cdevsw -
 *              mxopen, mxclose, mxread, mxwrite, mxioctl, 0,
 *
 *      also requires a line in linesw -
 *              mxopen, mxclose, mcread, mcwrite, mxioctl, nulldev, nulldev,
 *
 *      The linesw entry for mpx should be the last one in the table.
 *      'nldisp' (number of line disciplines) should not include the
 *      mpx line.  This is to prevent mpx from being enabled by an ioctl.

 *      mxtty.c must be loaded instead of tty.c so that certain
 *      sleeps will not be done if a typewriter is connected to
 *      a channel and so that sdata will be called from ttyinput.
 *      
 */
struct  chan    chans[NCHANS];
struct  schan   schans[NPORTS];
struct  group   *groups[NGROUPS];
int     mpxline;
struct chan *UNIX_xcp();
dev_t   mpxdev  = -1;


char    mcdebugs[NDEBUGS];


/*
 * Allocate a channel, set c_index to index.
 */
struct  chan *
UNIX_challoc(index, isport)
{
register s;
register struct chan *cp, *lastcp;

        s = spl6();
        if (isport) {
                cp = (struct chan *)schans;
                lastcp = (struct chan *)&schans[NPORTS];
        } else {
                cp = chans;
                lastcp = &chans[NCHANS];
        }

        for(; cp < lastcp; cp++) 
                if(cp->c_group==NULL) {
                        cp->c_index = index;
                        cp->c_pgrp = 0;
                        cp->c_flags = 0;
                        splx(s);
                        return(cp);
                }
        splx(s);
        return(NULL);
}



/*
 * Allocate a group table cell.
 */
UNIX_gpalloc()
{
register i,s;

        s = spl6();
        for(i = NGROUPS-1; i>=0; i--)
                if (groups[i]==NULL) {
                        groups[i]++;
                        break;
                }
        splx(s);
        return(i);
}


/*
 * Add a channel to the group in
 * inode ip.
 */
struct chan *
UNIX_addch(ip, isport)
struct inode *ip;
{
register struct chan *cp;
register struct group *gp;
register i;

        UNIX_plock(ip);
        gp = &ip->i_un.b.i_group;
        for(i=0;i<NINDEX;i++) {
                cp = (struct chan *)gp->g_chans[i];
                if (cp == NULL) {
                        if ((cp=UNIX_challoc(i, isport)) != NULL) {
                                gp->g_chans[i] = cp;
                                cp->c_group = gp;
                        }
                        break;
                }
                cp = NULL;
        }
        UNIX_prele(ip);
        return(cp);
}




/*
 * mpxchan system call
 */
UNIX_mpxchan()
{
struct  inode *ip, *gip;
register int i;
extern  UNIX_mxopen(), UNIX_mcread();
extern  UNIX_sdata(), UNIX_scontrol();
dev_t   dev;
struct  tty *tp;
struct  file *fp, *chfp, *gfp;
struct  chan *cp;
struct  group *gp, *ngp;
struct  a {
        int     cmd;
        int     *argvec;
} *uap;
struct mx_args vec;


        /*
         * common setup code.
         */
        uap = (struct a *)u.u_ap;
        UNIX_copyin(uap->argvec, &vec, sizeof vec);
        gp = NULL;
        switch(uap->cmd) {
        case NPGRP:
                if (vec.m_arg[1] < 0)
                        goto sw;
        case CHAN:
        case JOIN:
        case EXTR:
        case ATTACH:
        case DETACH:
        case CSIG:
                gfp = UNIX_getf(vec.m_arg[1]);
                if (gfp==NULL)
                        goto bad;
                gip = gfp->f_inode;
                gp = &gip->i_un.b.i_group;
                if (gp->g_inode != gip)
                        goto bad;
        }

sw:
        switch(uap->cmd) {
        /*
         * creat an mpx file.
         */
        case MPX:
        case MPXN:
                if (mpxdev < 0) {
                        for(i=0; linesw[i].l_open; i++) 
                                if (linesw[i].l_read == UNIX_mcread) {
                                        mpxline = i;
                                        goto found1;
                                }
                        goto bad;

                found1:
                        for(i=0; cdevsw[i].d_open; i++) {
                                if (cdevsw[i].d_open == UNIX_mxopen)
                                        goto found2;
                        }
                bad:
                        u.u_error = ENXIO;
                        return;
                found2:
                        mpxdev = (dev_t)(i<<8);
                }
                if (uap->cmd==MPXN) {
                        if ((ip=UNIX_ialloc(rootdev))==NULL)
                                goto bad;
                        ip->i_mode = (vec.m_arg[1]&0777)+IFCHR;
                        ip->i_flag = IACC|IUPD|ICHG;
                        goto merge;
                }
                u.u_dirp = vec.m_name;
                ip = UNIX_namei(UNIX_uchar,1);
                if (ip != NULL) {
                        u.u_error = EEXIST;
                        UNIX_iput(ip);
                        return;
                }
                if (u.u_error)
                        return;
                ip = UNIX_maknode((vec.m_arg[1]&0777)+IFCHR);
                if (ip == NULL)
                        return;
        merge:
                if ((i = UNIX_gpalloc()) < 0) {
                        UNIX_iput(ip);
                        goto bad;
                }
                ip->i_un.b.i_rdev = (daddr_t)(mpxdev+i);
                gp = &ip->i_un.b.i_group;
                groups[i] = gp;
                gp->g_inode = ip;
                gp->g_state = COPEN;
                gp->g_group = NULL;
                gp->g_index = 0;
                gp->g_rotmask = 1;
                gp->g_rot = 0;
                gp->g_datq = 0;
                UNIX_open1(ip,FREAD+FWRITE,2);
                if (u.u_error) {
                        groups[i] = NULL;
                        UNIX_iput(ip);
                        goto bad;
                }
                ip->i_mode = (ip->i_mode & ~IFMT) | IFMPC;
                ip->i_count++;
                fp = u.u_ofile[u.u_r.a.r_val1];
                fp->f_flag |= FMP;
                fp->f_un.f_chan = NULL;
                gp->g_file = fp;
                for(i=0;i<NINDEX;)
                        gp->g_chans[i++] = NULL;
                return;
        /*
         * join file descriptor (arg 0) to group (arg 1)
         * return channel number
         */
        case JOIN:
                if ((fp=UNIX_getf(vec.m_arg[0]))==NULL)
                        goto bad;
                ip = fp->f_inode;
                i = ip->i_mode & IFMT;
                if (i == IFMPC) {
                        if ((fp->f_flag&FMP) != FMP) {
                                goto bad;
                        }
                        ngp = &ip->i_un.b.i_group;
                        UNIX_mlink (ngp, gp);
                        fp->f_count++;
                        return;
                }
                if (i != IFCHR) 
                        goto bad;
                dev = (dev_t)ip->i_un.b.i_rdev;
                tp = cdevsw[major(dev)].d_ttys;
                if (tp==NULL)
                        goto bad;
                tp = &tp[minor(dev)];
                if (tp->t_chan)
                        goto bad;
                if ((cp=UNIX_addch(gip, 1))==NULL)
                        goto bad;
                tp->t_chan = cp;
                cp->c_fy = fp;
                fp->f_count++;
                cp->c_ttyp = tp;
                cp->c_line = tp->t_line;
                cp->c_flags = XGRP+PORT;
                u.u_r.a.r_val1 = UNIX_cpx(cp);
                return;
        /*
         * attach channel (arg 0) to group (arg 1)
         */
        case ATTACH:
                cp = UNIX_xcp(gp, vec.m_arg[0]);
                if (cp==NULL)
                        goto bad;
                u.u_r.a.r_val1 = UNIX_cpx(cp);
                UNIX_wakeup((caddr_t)cp);
                return;
        case DETACH:
                cp = UNIX_xcp(gp, vec.m_arg[0]);
                if (cp==NULL)
                        goto bad;
                UNIX_detach(cp);
                return;
        /*
         * extract channel (arg 0) from group (arg 1).
         */
        case EXTR:
                cp = UNIX_xcp(gp, vec.m_arg[0]);
                if (cp==NULL) {
                        goto bad;
                }
                if (cp->c_flags & ISGRP) {
                        UNIX_mxfalloc(((struct group *)cp)->g_file);
                        return;
                }
                if ((fp = cp->c_fy) != NULL) {
                        UNIX_mxfalloc(fp);
                        return;
                }
                if ((fp = UNIX_falloc()) == NULL) {
                        return;
                }
                fp->f_inode = gip;
                gip->i_count++;
                fp->f_un.f_chan = cp;
                fp->f_flag = (vec.m_arg[2]) ?
                                (FREAD|FWRITE|FMPY) : (FREAD|FWRITE|FMPX);
                cp->c_fy = fp;
                return;
        /*
         * make new chan on group (arg 1)
         */
        case CHAN:
                if ((cp=UNIX_addch(gip, 0))==NULL)
                        goto bad;
                cp->c_flags = XGRP;
                cp->c_fy = NULL;
                cp->c_ttyp = cp->c_ottyp = (struct tty *)cp;
                cp->c_line = cp->c_oline = mpxline;
                u.u_r.a.r_val1 = UNIX_cpx(cp);
                return;
        /*
         * connect fd (arg 0) to channel fd (arg 1)
         * (arg 2 <  0) => fd to chan only
         * (arg 2 >  0) => chan to fd only
         * (arg 2 == 0) => both directions
         */
        case CONNECT:
                if ((fp=UNIX_getf(vec.m_arg[0]))==NULL)
                        goto bad;
                if ((chfp=UNIX_getf(vec.m_arg[1]))==NULL)
                        goto bad;
                ip = fp->f_inode;
                i = ip->i_mode&IFMT;
                if (i!=IFCHR)
                        goto bad;
                dev = (dev_t)ip->i_un.b.i_rdev;
                tp = cdevsw[major(dev)].d_ttys;
                if (tp==NULL)
                        goto bad;
                tp = &tp[minor(dev)];
                if (!(chfp->f_flag&FMPY)) {
                        goto bad;
                }
                cp = chfp->f_un.f_chan;
                if (cp==NULL || cp->c_flags&PORT) {
                        goto bad;
                }
                i = vec.m_arg[2];
                if (i>=0) {
                        cp->c_ottyp = tp;
                        cp->c_oline = tp->t_line;
                }
                if (i<=0)  {
                        tp->t_chan = cp;
                        cp->c_ttyp = tp;
                        cp->c_line = tp->t_line;
                }
                return;
        case NPGRP: {
                register struct proc *pp;

                if (gp != NULL) {
                        cp = UNIX_xcp(gp, vec.m_arg[0]);
                        if (cp==NULL)
                                goto bad;
                }
                pp = u.u_procp;
                pp->p_pgrp = pp->p_pid;
                if (vec.m_arg[2])
                        pp->p_pgrp = vec.m_arg[2];
                if (gp != NULL)
                        cp->c_pgrp = pp->p_pgrp;
                return;
        }
        case CSIG:
                cp = UNIX_xcp(gp, vec.m_arg[0]);
                if (cp==NULL)
                        goto bad;
                UNIX_signal(cp->c_pgrp, vec.m_arg[2]);
                return;
        case DEBUG:
                i = vec.m_arg[0];
                if (i<0 || i>NDEBUGS)
                        return;
                mcdebugs[i] = vec.m_arg[1];
                if (i==ALL)
                        for(i=0;i<NDEBUGS;i++)
                                mcdebugs[i] = vec.m_arg[1];
                return;
        default:
                goto bad;
        }
}

UNIX_detach(cp)
register struct chan *cp;
{
        register struct group *gp;
        register int i;

        if (cp->c_flags&ISGRP) {
                gp = (struct group *)cp;
                UNIX_closef(gp->g_file);
                i = ((struct group *)cp)->g_index;
                gp->g_chans[i] = NULL;
                return;
        } else if (cp->c_flags&PORT && cp->c_ttyp != NULL) {
                UNIX_closef(cp->c_fy);
                UNIX_chdrain(cp);
                UNIX_chfree(cp);
                return;
        }
        if (cp->c_fy && (cp->c_flags&WCLOSE)==0) {
                cp->c_flags |= WCLOSE;
                UNIX_chwake(cp);
        } else {
                UNIX_chdrain(cp);
                UNIX_chfree(cp);
        }
}


UNIX_mxfalloc(fp)
register struct file *fp;
{
register i;

        if (fp==NULL) {
                u.u_error = ENXIO;
                return(-1);
        }

        i = UNIX_ufalloc();
        if (i < 0)
                return(i);
        u.u_ofile[i] = fp;
        fp->f_count++;
        u.u_r.a.r_val1 = i;
        return(i);
}




UNIX_mlink(sub,master)
struct group *sub, *master;
{
register i;


        for(i=0;i<NINDEX;i++) {
                if (master->g_chans[i] != NULL)
                        continue;
                master->g_chans[i] = (struct chan *)sub;
                sub->g_group = master;
                sub->g_index = i;
                u.u_r.a.r_val1 = i;
                return;
        }
        u.u_error = ENXIO;
        return;
}
