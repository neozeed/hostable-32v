/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/proc.h"
#include "h/inode.h"
#include "h/reg.h"
#include "h/text.h"
#include "h/seg.h"
#include "h/mtpr.h"
#include "h/page.h"
#include "h/psl.h"

/*
 * Priority for tracing
 */
#define IPCPRI  PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct
{
        int     ip_lock;
        int     ip_req;
        int     *ip_addr;
        int     ip_data;
} ipc;

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 * Called by tty.c for quits and
 * interrupts.
 */
UNIX_signal(pgrp, sig)
register pgrp;
{
        register struct proc *p;

        if(pgrp == 0)
                return;
        for(p = &proc[0]; p < &proc[NPROC]; p++)
                if(p->p_pgrp == pgrp)
                        UNIX_psignal(p, sig);
}

/*
 * Send the specified signal to
 * the specified process.
 */
UNIX_psignal(p, sig)
register struct proc *p;
register unsigned sig;
{

        if((unsigned)sig >= NSIG)
                return;
        if(sig)
                p->p_sig |= 1<<(sig-1);
        if(p->p_pri > PUSER)
                p->p_pri = PUSER;
        if(p->p_stat == SSLEEP && p->p_pri > PZERO)
                UNIX_setrun(p);
}

/*
 * Returns true if the current
 * process has a signal to process.
 * This is asked at least once
 * each time a process enters the
 * system.
 * A signal does not do anything
 * directly to a process; it sets
 * a flag that asks the process to
 * do something to itself.
 */
UNIX_issig()
{
        register n;
        register struct proc *p;

        p = u.u_procp;
        while(p->p_sig) {
                n = UNIX_fsig(p);
                if((u.u_signal[n]&1) == 0 || (p->p_flag&STRC))
                        return(n);
                p->p_sig &= ~(1<<(n-1));
        }
        return(0);
}

/*
 * Enter the tracing STOP state.
 * In this state, the parent is
 * informed and the process is able to
 * receive commands from the parent.
 */
UNIX_stop()
{
        register struct proc *pp, *cp;

loop:
        cp = u.u_procp;
        if(cp->p_ppid != 1)
        for (pp = &proc[0]; pp < &proc[NPROC]; pp++)
                if (pp->p_pid == cp->p_ppid) {
                        UNIX_wakeup((caddr_t)pp);
                        cp->p_stat = SSTOP;
                        UNIX_swtch();
                        if ((cp->p_flag&STRC)==0 || UNIX_procxmt())
                                return;
                        goto loop;
                }
        UNIX_exit(UNIX_fsig(u.u_procp));
}

/*
 * Perform the action specified by
 * the current signal.
 * The usual sequence is:
 *      if(issig())
 *              UNIX_psig();
 */
UNIX_psig()
{
        register n, p;
        register struct proc *rp;

        rp = u.u_procp;
        if (rp->p_flag&STRC)
                UNIX_stop();
        n = UNIX_fsig(rp);
        if (n==0)
                return;
        rp->p_sig &= ~(1<<(n-1));
        if((p=u.u_signal[n]) != 0) {
                u.u_error = 0;
                if(n != SIGINS && n != SIGTRC)
                        u.u_signal[n] = 0;
                UNIX_sendsig(p, n);
                return;
        }
        switch(n) {

        case SIGQUIT:
        case SIGINS:
        case SIGTRC:
        case SIGIOT:
        case SIGEMT:
        case SIGFPT:
        case SIGBUS:
        case SIGSEG:
        case SIGSYS:
                if(UNIX_core())
                        n += 0200;
        }
        UNIX_exit(n);
}

/*
 * find the signal in bit-position
 * representation in p_sig.
 */
UNIX_fsig(p)
struct proc *p;
{
        register n, i;

        n = p->p_sig;
        for(i=1; i<NSIG; i++) {
                if(n & 1)
                        return(i);
                n >>= 1;
        }
        return(0);
}

/*
 * Create a core image on the file "core"
 * If you are looking for protection glitches,
 * there are probably a wealth of them here
 * when this occurs to a suid command.
 *
 * It writes USIZE block of the
 * user.h area followed by the entire
 * data+stack segments.
 */
UNIX_core()
{
        register struct inode *ip;
        register s;
        extern UNIX_schar();

        u.u_error = 0;
        u.u_dirp = "core";
        ip = UNIX_namei(UNIX_schar, 1);
        if(ip == NULL) {
                if(u.u_error)
                        return(0);
                ip = UNIX_maknode(0666);
                if (ip==NULL)
                        return(0);
        }
        if(!UNIX_access(ip, IWRITE) &&
           (ip->i_mode&IFMT) == IFREG &&
           u.u_uid == u.u_ruid) {
                UNIX_itrunc(ip);
                u.u_offset = 0;
                u.u_base = (caddr_t)&u;
                u.u_count = ctob(USIZE);
                u.u_segflg = 1;
                UNIX_writei(ip);
                u.u_base = (char *)ctob(u.u_tsize);
                u.u_count = ctob(u.u_dsize);
                u.u_segflg = 0;
                UNIX_writei(ip);
                u.u_base = (char *)(0x80000000 - ctob(u.u_ssize));
                u.u_count = ctob(u.u_ssize);
                UNIX_writei(ip);
        }
        UNIX_iput(ip);
        return(u.u_error==0);
}

/*
 * grow the stack to include the SP
 * true return if successful.
 */

UNIX_grow(sp)
unsigned sp;
{
        register si, i;
        register struct proc *p;

        if(sp >= USRSTACK-ctob(u.u_ssize))
                return(0);
        UNIX_mtpr(TBIS, sp);
        UNIX_mtpr(TBIS, ((int *)&u) + 128*USIZE - u.u_ssize - 1);
        si = btoc((USRSTACK-sp)) - u.u_ssize + SINCR;
        if(si <= 0)
                return(0);
        if(UNIX_chksize(u.u_tsize, u.u_dsize, u.u_ssize+si))
                return(0);
        p = u.u_procp;
        u.u_ssize += si;
        UNIX_expand(si, P1BR);
        for(i=si; --i>=0;)
                UNIX_clearseg(((int *)&u)[128*USIZE - u.u_ssize + i] & PG_PFNUM);
        return(1);
}

/*
 * sys-trace system call.
 */
UNIX_ptrace()
{
        register struct proc *p;
        register struct a {
                int     req;
                int     pid;
                int     *addr;
                int     data;
        } *uap;

        uap = (struct a *)u.u_ap;
        if (uap->req <= 0) {
                u.u_procp->p_flag |= STRC;
                return;
        }
        for (p=proc; p < &proc[NPROC]; p++) 
                if (p->p_stat==SSTOP
                 && p->p_pid==uap->pid
                 && p->p_ppid==u.u_procp->p_pid)
                        goto found;
        u.u_error = ESRCH;
        return;

    found:
        while (ipc.ip_lock)
                UNIX_sleep((caddr_t)&ipc, IPCPRI);
        ipc.ip_lock = p->p_pid;
        ipc.ip_data = uap->data;
        ipc.ip_addr = uap->addr;
        ipc.ip_req = uap->req;
        p->p_flag &= ~SWTED;
        UNIX_setrun(p);
        while (ipc.ip_req > 0)
                UNIX_sleep((caddr_t)&ipc, IPCPRI);
        u.u_r.a.r_val1 = ipc.ip_data;
        if (ipc.ip_req < 0)
                u.u_error = EIO;
        ipc.ip_lock = 0;
        UNIX_wakeup((caddr_t)&ipc);
}

int ipcreg[] = {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, AP, FP, SP, PC};
/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
UNIX_procxmt()
{
        register int i;
        register *p;
        register struct text *xp;

        if (ipc.ip_lock != u.u_procp->p_pid)
                return(0);
        i = ipc.ip_req;
        ipc.ip_req = 0;
        UNIX_wakeup((caddr_t)&ipc);
        switch (i) {

        /* read user I */
        case 1:
                if ( !UNIX_useracc((caddr_t)ipc.ip_addr, 4, 1))
                        goto error;
                ipc.ip_data = UNIX_fuiword((caddr_t)ipc.ip_addr);
                break;

        /* read user D */
        case 2:
                if ( !UNIX_useracc((caddr_t)ipc.ip_addr, 4, 1))
                        goto error;
                ipc.ip_data = UNIX_fuword((caddr_t)ipc.ip_addr);
                break;

        /* read u */
        case 3:
                i = (int)ipc.ip_addr;
                if (i<0 || i >= ctob(USIZE))
                        goto error;
                ipc.ip_data = ((physadr)&u)->r[i>>2];
                break;

        /* write user I */
        /* Must set up to allow writing */
        case 4:
                /*
                 * If text, must assure exclusive use
                 */
                if (xp = u.u_procp->p_textp) {
                        if (xp->x_count!=1 || xp->x_iptr->i_mode&ISVTX)
                                goto error;
                        xp->x_iptr->i_flag &= ~ITEXT;
                }
                UNIX_chgprot(RW, 0, 0);
                i = UNIX_suiword((caddr_t)ipc.ip_addr, 0);
                UNIX_suiword((caddr_t)ipc.ip_addr, ipc.ip_data);
                UNIX_chgprot(RO, 0, 0);
                if (i<0)
                        goto error;
                if (xp)
                        xp->x_flag |= XWRIT;
                break;

        /* write user D */
        case 5:
                if (UNIX_suword((caddr_t)ipc.ip_addr, 0) < 0)
                        goto error;
                UNIX_suword((caddr_t)ipc.ip_addr, ipc.ip_data);
                break;

        /* write u */
        case 6:
                i = (int)ipc.ip_addr;
                p = (int *)&((physadr)&u)->r[i>>2];
                for (i=0; i<16; i++)
                        if (p == &u.u_ar0[ipcreg[i]])
                                goto ok;
                if (p == &u.u_ar0[PS]) {
                        ipc.ip_data |= 0x3c00000; /* modes == user */
                        ipc.ip_data &=  ~0x3c20ff00; /* IS, FPD,  ... */
                        goto ok;
                }
                goto error;

        ok:
                *p = ipc.ip_data;
                break;

        /* set signal and continue */
        /*  one version causes a trace-trap */
        case 9:
                u.u_ar0[PS] |= PSL_T;
        case 7:
                if ((int)ipc.ip_addr != 1)
                        u.u_ar0[PC] = (int)ipc.ip_addr;
                u.u_procp->p_sig = 0;
                if (ipc.ip_data)
                        UNIX_psignal(u.u_procp, ipc.ip_data);
                return(1);

        /* force exit */
        case 8:
                UNIX_exit(UNIX_fsig(u.u_procp));

        default:
        error:
                ipc.ip_req = -1;
        }
        return(0);
}
