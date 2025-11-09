/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/
/*
 *   KL/DL-11 driver
 */
#include "h/param.h"
#include "h/conf.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/tty.h"
#include "h/systm.h"
#include "h/cons.h"
#include "h/mtpr.h"

#define NL1     000400
#define NL2     001000
#define CR2     020000
#define FF1     040000
#define TAB1    002000

struct  tty cons;
int     UNIX_consstart();
int     UNIX_ttrstrt();
char    partab[];

UNIX_consopen(dev, flag)
dev_t dev;
{
        register struct device *addr;
        register struct tty *tp;
        register d;

        tp = &cons;
        tp->t_oproc = UNIX_consstart;
        tp->t_iproc = NULL;
        if ((tp->t_state&ISOPEN) == 0) {
                tp->t_state = ISOPEN|CARR_ON;
                tp->t_flags = EVENP|ECHO|XTABS|CRMOD;
                tp->t_erase = CERASE;
                tp->t_kill = CKILL;
        }
        UNIX_mtpr(RXCS, UNIX_mfpr(RXCS)|RXCS_IE);
        UNIX_mtpr(TXCS, UNIX_mfpr(TXCS)|TXCS_IE);
        UNIX_ttyopen(dev, tp);
}

UNIX_consclose(dev)
dev_t dev;
{
        register struct tty *tp;

        tp = &cons;
        UNIX_wflushtty(tp);
        tp->t_state = 0;
}

UNIX_consread(dev)
dev_t dev;
{
        UNIX_ttread(&cons);
}

UNIX_conswrite(dev)
dev_t dev;
{
        UNIX_ttwrite(&cons);
}

UNIX_consxint(dev)
dev_t dev;
{
        register struct tty *tp;

        tp = &cons;
        UNIX_ttstart(tp);
        if (tp->t_outq.c_cc == 0 || tp->t_outq.c_cc == TTLOWAT)
                UNIX_wakeup((caddr_t)&tp->t_outq);
}

UNIX_consrint(dev)
dev_t dev;
{
        register int c;
        register struct device *addr;
        register struct tty *tp;

        c = UNIX_mfpr(RXDB);
        UNIX_ttyinput(c, &cons);
}

UNIX_consioctl(dev,cmd,addr,flag)
dev_t dev;
caddr_t addr;
{
        register struct tty *tp;
 
        tp = &cons;
        if (UNIX_ttioccom(cmd,tp,addr,dev) ==0)
                u.u_error = ENOTTY;
}

UNIX_consstart(tp)
register struct tty *tp;
{
        register c;
        register struct device *addr;

        if( (UNIX_mfpr(TXCS)&TXCS_RDY) == 0)
                return;
        if ((c=UNIX_getc(&tp->t_outq)) >= 0) {
                if (tp->t_flags&RAW)
                        UNIX_mtpr(TXDB, c&0xff);
                else if (c<=0177)
                        UNIX_mtpr(TXDB, (c | (partab[c]&0200))&0xff);
                else {
                        UNIX_timeout(UNIX_ttrstrt, (caddr_t)tp, (c&0177));
                        tp->t_state |= TIMEOUT;
                }
        }
}

char    *msgbufp = msgbuf;      /* Next saved printf character */
/*
 * Print a character on console.
 * Attempts to save and restore device
 * status.
 * If the switches are 0, all
 * printing is inhibited.
 *
 * Whether or not printing is inhibited,
 * the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
UNIX_putchar(c)
register c;
{
        register s, timo;
/*THIS NEEDS FIXING FOR NATIVE FUNCTIONS BIGTIME! BUT WE CAN SEE THE PANIC FOR DEVTAB!
        if (c != '\0' && c != '\r' && c != 0177) {
                *msgbufp++ = c;
                if(msgbufp >= &msgbuf[MSGBUFS])
                        msgbufp = msgbuf;
        }
        timo = 30000;
        *
         * Try waiting for the console tty to come ready,
         * otherwise give up after a reasonable time.
         *
        while((UNIX_mfpr(TXCS)&TXCS_RDY) == 0)
                if(--timo == 0)
                        break;
        if(c == 0)
                return;
        s = UNIX_mfpr(TXCS);
        UNIX_mtpr(TXCS,0);
        UNIX_mtpr(TXDB, c&0xff);
        if(c == '\n') {
                UNIX_putchar('\r');
        }
        UNIX_putchar(0);
        UNIX_mtpr(TXCS, s);
        */
        printf("%c",c);
}
