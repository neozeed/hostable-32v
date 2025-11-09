/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/
/*
 * general TTY subroutines
 */
#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/tty.h"
#include "h/proc.h"
#include "h/mx.h"
#include "h/inode.h"
#include "h/file.h"
#include "h/reg.h"
#include "h/conf.h"

char    partab[];

/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */

char    maptab[] ={
        000,000,000,000,CEOT,00,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,'|',000,000,000,000,000,'`',
        '{','}',000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,000,000,
        000,000,000,000,000,000,'~',000,
        000,'A','B','C','D','E','F','G',
        'H','I','J','K','L','M','N','O',
        'P','Q','R','S','T','U','V','W',
        'X','Y','Z',000,000,000,000,000,
};

/*
 * routine called on first teletype open.
 * establishes a process group for distribution
 * of quits and interrupts from the tty.
 */
UNIX_ttyopen(dev, tp)
dev_t dev;
register struct tty *tp;
{
        register struct proc *pp;

        pp = u.u_procp;
        if(pp->p_pgrp == 0) {
                u.u_ttyp = tp;
                u.u_ttyd = dev;
                if (tp->t_pgrp==0)
                        tp->t_pgrp = pp->p_pid;
                pp->p_pgrp = tp->t_pgrp;
        }
        tp->t_state &= ~WOPEN;
        tp->t_state |= ISOPEN;
}

/*
 * clean tp on last close
 */
UNIX_ttyclose(tp)
register struct tty *tp;
{

        tp->t_pgrp = 0;
        UNIX_wflushtty(tp);
        tp->t_state = 0;
}

/*
 * stty/gtty writearound
 */
UNIX_stty()
{
        u.u_arg[2] = u.u_arg[1];
        u.u_arg[1] = TIOCSETP;
        UNIX_ioctl();
}

UNIX_gtty()
{
        u.u_arg[2] = u.u_arg[1];
        u.u_arg[1] = TIOCGETP;
        UNIX_ioctl();
}

/*
 * ioctl system call
 * Check legality, execute common code, and switch out to individual
 * device routine.
 */
UNIX_ioctl()
{
        register struct file *fp;
        register struct inode *ip;
        register struct a {
                int     fdes;
                int     cmd;
                caddr_t cmarg;
        } *uap;
        register dev_t dev;
        register fmt;

        uap = (struct a *)u.u_ap;
        if ((fp = UNIX_getf(uap->fdes)) == NULL)
                return;
        if (uap->cmd==FIOCLEX) {
                u.u_pofile[uap->fdes] |= EXCLOSE;
                return;
        }
        if (uap->cmd==FIONCLEX) {
                u.u_pofile[uap->fdes] &= ~EXCLOSE;
                return;
        }
        ip = fp->f_inode;
        fmt = ip->i_mode & IFMT;
        if (fmt != IFCHR && fmt != IFMPC) {
                u.u_error = ENOTTY;
                return;
        }
        dev = (dev_t)ip->i_un.b.i_rdev;
        (*cdevsw[major(dev)].d_ioctl)(dev, uap->cmd, uap->cmarg, fp->f_flag&(FREAD|FWRITE));
}

/*
 * Common code for several tty ioctl commands
 */
UNIX_ttioccomm(com, tp, addr, dev)
register struct tty *tp;
caddr_t addr;
{
        unsigned t;
        struct ttiocb iocb;
        extern int nldisp;

        switch(com) {

        /*
         * get discipline number
         */
        case TIOCGETD:
                t = tp->t_line;
                if (UNIX_copyout((caddr_t)&t, addr, sizeof(t)))
                        u.u_error = EFAULT;
                break;

        /*
         * set line discipline
         */
        case TIOCSETD:
                if (UNIX_copyin(addr, (caddr_t)&t, sizeof(t))) {
                        u.u_error = EFAULT;
                        break;
                }
                if (t >= nldisp) {
                        u.u_error = ENXIO;
                        break;
                }
                if (tp->t_line)
                        (*linesw[tp->t_line].l_close)(tp);
                if (t)
                        (*linesw[t].l_open)(dev, tp, addr);
                if (u.u_error==0)
                        tp->t_line = t;
                break;

        /*
         * prevent more opens on channel
         */
        case TIOCEXCL:
                tp->t_state |= XCLUDE;
                break;
        case TIOCNXCL:
                tp->t_state &= ~XCLUDE;
                break;

        /*
         * Set new parameters
         */
        case TIOCSETP:
                UNIX_wflushtty(tp);
        case TIOCSETN:
                if (UNIX_copyin(addr, (caddr_t)&iocb, sizeof(iocb))) {
                        u.u_error = EFAULT;
                        return(1);
                }
                tp->t_ispeed = iocb.ioc_ispeed;
                tp->t_ospeed = iocb.ioc_ospeed;
                tp->t_erase = iocb.ioc_erase;
                tp->t_kill = iocb.ioc_kill;
                tp->t_flags = iocb.ioc_flags;
                break;

        /*
         * send current parameters to user
         */
        case TIOCGETP:
                iocb.ioc_ispeed = tp->t_ispeed;
                iocb.ioc_ospeed = tp->t_ospeed;
                iocb.ioc_erase = tp->t_erase;
                iocb.ioc_kill = tp->t_kill;
                iocb.ioc_flags = tp->t_flags;
                if (UNIX_copyout((caddr_t)&iocb, addr, sizeof(iocb)))
                        u.u_error = EFAULT;
                break;

        /*
         * Hang up line on last close
         */

        case TIOCHPCL:
                tp->t_state |= HUPCLS;
                break;

        /*
         * toggle TTSTOP from user program
         */
        case TIOCTSTP:
                tp->t_state ^= TTSTOP;
                UNIX_ttstart(tp);
                break;

        /*
         * ioctl entries to line discipline
         */
        case DIOCSETP:
        case DIOCGETP:
                (*linesw[tp->t_line].l_ioctl)(com, tp, addr);
                break;
        default:
                return(0);
        }
        return(1);
}

/*
 * Wait for output to drain, then flush input waiting.
 */
UNIX_wflushtty(tp)
register struct tty *tp;
{

        spl5();
        while (tp->t_outq.c_cc && tp->t_state&CARR_ON) {
                (*tp->t_oproc)(tp);
                tp->t_state |= ASLEEP;
                UNIX_sleep((caddr_t)&tp->t_outq, TTOPRI);
        }
        UNIX_flushtty(tp);
        spl0();
}

/*
 * flush all TTY queues
 */
UNIX_flushtty(tp)
register struct tty *tp;
{
        register s;

        while (UNIX_getc(&tp->t_canq) >= 0)
                ;
        while (UNIX_getc(&tp->t_outq) >= 0)
                ;
        UNIX_wakeup((caddr_t)&tp->t_rawq);
        UNIX_wakeup((caddr_t)&tp->t_outq);
        s = spl5();
        while (UNIX_getc(&tp->t_rawq) >= 0)
                ;
        tp->t_delct = 0;
        splx(s);
}

/*
 * transfer raw input list to canonical list,
 * doing erase-kill processing and handling escapes.
 * It waits until a full line has been typed in cooked mode,
 * or until any character has been typed in raw mode.
 */
UNIX_canon(tp)
register struct tty *tp;
{
        register char *bp;
        char *bp1;
        register int c;
        int mc;

        spl5();
        while ((tp->t_flags&(RAW|CBREAK))==0 && tp->t_delct==0
            || (tp->t_flags&(RAW|CBREAK))!=0 && tp->t_rawq.c_cc==0) {
                if ((tp->t_state&CARR_ON)==0 || tp->t_chan!=NULL) {
                        return(0);
                }
                UNIX_sleep((caddr_t)&tp->t_rawq, TTIPRI);
        }
        spl0();
loop:
        bp = &canonb[2];
        while ((c=UNIX_getc(&tp->t_rawq)) >= 0) {
                if ((tp->t_flags&(RAW|CBREAK))==0) {
                        if (c==0377) {
                                tp->t_delct--;
                                break;
                        }
                        if (bp[-1]!='\\') {
                                if (c==tp->t_erase) {
                                        if (bp > &canonb[2])
                                                bp--;
                                        continue;
                                }
                                if (c==tp->t_kill)
                                        goto loop;
                                if (c==CEOT)
                                        continue;
                        } else {
                                mc = maptab[c] & 0177;
                                if (c==tp->t_erase || c==tp->t_kill)
                                        mc = c;
                                if (mc && (mc==c || (tp->t_flags&LCASE))) {
                                        if (bp[-2] != '\\')
                                                c = mc;
                                        bp--;
                                }
                        }
                }
                *bp++ = c;
                if (bp>=canonb+CANBSIZ)
                        break;
        }
        bp1 = bp;
        bp = &canonb[2];
        while (bp<bp1)
                UNIX_putc(*bp++, &tp->t_canq);


        if (tp->t_state&TBLOCK && tp->t_rawq.c_cc < TTYHOG/5) 
                if (UNIX_putc(CSTOP, &tp->t_outq)==0) {
                        tp->t_state &= ~TBLOCK;
                        UNIX_ttstart(tp);
                }


        return(bp1 - &canonb[2]);
}

/*
 * Place a character on raw TTY input queue, putting in delimiters
 * and waking up top half as needed.
 * Also echo if required.
 * The arguments are the character and the appropriate
 * tty structure.
 */
UNIX_ttyinput(c, tp)
register c;
register struct tty *tp;
{
        register int t_flags;
        register struct chan *cp;

        tk_nin += 1;
        c &= 0377;
        t_flags = tp->t_flags;
        if (t_flags&TANDEM) {
                if (tp->t_rawq.c_cc >= TTYHOG/2 && (tp->t_state&TBLOCK)==0) {
                        if (UNIX_putc(CSTOP, &tp->t_outq)==0) {
                                tp->t_state |= TBLOCK;
                                UNIX_ttstart(tp);
                        }
                }
        }
        if ((t_flags&RAW)==0) {
                c &= 0177;
                if (c==CSTOP) {
                        tp->t_state ^= TTSTOP;
                        UNIX_ttstart(tp);
                        return;
                }
                tp->t_state &= ~TTSTOP;
                if (c==CQUIT || c==CINTR) {
                        UNIX_signal(tp->t_pgrp, c==CINTR? SIGINT:SIGQUIT);
                        UNIX_flushtty(tp);
                        return;
                }
                if (c=='\r' && t_flags&CRMOD)
                        c = '\n';
        }
        if (tp->t_rawq.c_cc>TTYHOG) {
                UNIX_flushtty(tp);
                return;
        }
        if (t_flags&LCASE && c>='A' && c<='Z')
                c += 'a'-'A';
        UNIX_putc(c, &tp->t_rawq);
        if (t_flags&(RAW|CBREAK) || (c=='\n' || c==CEOT)) {
                if ((t_flags&(RAW|CBREAK))==0 && UNIX_putc(0377, &tp->t_rawq)==0)
                        tp->t_delct++;
                        if ((cp=tp->t_chan)!=NULL)
                                UNIX_sdata(cp); else
                                UNIX_wakeup((caddr_t)&tp->t_rawq);
        }
        if (t_flags&ECHO) {
                UNIX_ttyoutput(c, tp);
                if (c==tp->t_kill && (t_flags&(RAW|CBREAK))==0)
                        UNIX_ttyoutput('\n', tp);
                UNIX_ttstart(tp);
        }
}

/*
 * put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * It is called both from the top half for output, and from
 * interrupt level for echoing.
 * The arguments are the character and the tty structure.
 */
UNIX_ttyoutput(c, tp)
register c;
register struct tty *tp;
{
        register char *colp;
        register ctype;

        tk_nout += 1;
        /*
         * Ignore EOT in normal mode to avoid hanging up
         * certain terminals.
         * In raw mode dump the char unchanged.
         */

        if ((tp->t_flags&RAW)==0) {
                c &= 0177;
                if (c==CEOT)
                        return;
        } else {
                UNIX_putc(c, &tp->t_outq);
                return;
        }


        if (tp->t_flags&TANDEM && (c&0177)==CSTOP)
                return;


        /*
         * Turn tabs to spaces as required
         */
        if (c=='\t' && (tp->t_flags&TBDELAY)==XTABS) {
                c = 8;
                do
                        UNIX_ttyoutput(' ', tp);
                while (--c >= 0 && tp->t_col&07);
                return;
        }
        /*
         * for upper-case-only terminals,
         * generate escapes.
         */
        if (tp->t_flags&LCASE) {
                colp = "({)}!|^~'`";
                while(*colp++)
                        if(c == *colp++) {
                                UNIX_ttyoutput('\\', tp);
                                c = colp[-2];
                                break;
                        }
                if ('a'<=c && c<='z')
                        c += 'A' - 'a';
        }
        /*
         * turn <nl> to <cr><lf> if desired.
         */
        if (c=='\n' && tp->t_flags&CRMOD)
                UNIX_ttyoutput('\r', tp);
        UNIX_putc(c, &tp->t_outq);
        /*
         * Calculate delays.
         * The numbers here represent clock ticks
         * and are not necessarily optimal for all terminals.
         * The delays are indicated by characters above 0200.
         * In raw mode there are no delays and the
         * transmission path is 8 bits wide.
         */
        colp = &tp->t_col;
        ctype = partab[c];
        c = 0;
        switch (ctype&077) {

        /* ordinary */
        case 0:
                (*colp)++;

        /* non-printing */
        case 1:
                break;

        /* backspace */
        case 2:
                if (*colp)
                        (*colp)--;
                break;

        /* newline */
        case 3:
                ctype = (tp->t_flags >> 8) & 03;
                if(ctype == 1) { /* tty 37 */
                        if (*colp)
                                c = UNIX_max(((unsigned)*colp>>4) + 3, (unsigned)6);
                } else
                if(ctype == 2) { /* vt05 */
                        c = 6;
                }
                *colp = 0;
                break;

        /* tab */
        case 4:
                ctype = (tp->t_flags >> 10) & 03;
                if(ctype == 1) { /* tty 37 */
                        c = 1 - (*colp | ~07);
                        if(c < 5)
                                c = 0;
                }
                *colp |= 07;
                (*colp)++;
                break;

        /* vertical motion */
        case 5:
                if(tp->t_flags & VTDELAY) /* tty 37 */
                        c = 0177;
                break;

        /* carriage return */
        case 6:
                ctype = (tp->t_flags >> 12) & 03;
                if(ctype == 1) { /* tn 300 */
                        c = 5;
                } else if(ctype == 2) { /* ti 700 */
                        c = 10;
                }
                *colp = 0;
        }
        if(c)
                UNIX_putc(c|0200, &tp->t_outq);
}

/*
 * Restart typewriter output following a delay
 * timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
UNIX_ttrstrt(tp)
register struct tty *tp;
{

        tp->t_state &= ~TIMEOUT;
        UNIX_ttstart(tp);
}

/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 */
UNIX_ttstart(tp)
register struct tty *tp;
{
        register s;

        s = spl5();
        if((tp->t_state&(TIMEOUT|TTSTOP|BUSY)) == 0)
                (*tp->t_oproc)(tp);
        splx(s);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
UNIX_ttread(tp)
register struct tty *tp;
{
        struct chan *chan;

        if ((tp->t_state&CARR_ON)==0)
                return(0);
        if (tp->t_canq.c_cc || UNIX_canon(tp))
                while (tp->t_canq.c_cc && UNIX_passc(UNIX_getc(&tp->t_canq))>=0)
                        ;
out:
        return(tp->t_canq.c_cc);
}

/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 */
int TTIME       =100;
UNIX_ttwrite(tp)
register struct tty *tp;
{
        register c;

        if ((tp->t_state&CARR_ON)==0)
                return(0);
        while (u.u_count) {
                spl5();
                while (tp->t_outq.c_cc > TTIME) {
                        UNIX_ttstart(tp);
                        tp->t_state |= ASLEEP;
                        if (tp->t_chan) 
                                return((caddr_t)&tp->t_outq);
                        UNIX_sleep((caddr_t)&tp->t_outq, TTOPRI);
                }
                spl0();
                UNIX_ttyoutput(UNIX_cpass(), tp);
        }
        UNIX_ttstart(tp);
}
