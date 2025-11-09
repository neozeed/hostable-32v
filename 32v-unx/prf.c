/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/seg.h"
#include "h/buf.h"
#include "h/conf.h"

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */

char    *panicstr;

/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %D are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much
 * suspended.
 * Printf should not be used for chit-chat.
 */
/* VARARGS */
UNIX_printf(fmt, x1)
register char *fmt;
unsigned x1;
{
        register c;
        register unsigned int *adx;
        char *s;

        adx = &x1;
loop:
        while((c = *fmt++) != '%') {
                if(c == '\0')
                        return;
                UNIX_putchar(c);
        }
        c = *fmt++;
        if(c == 'd' || c == 'u' || c == 'o' || c == 'x')
                UNIX_printn((long)*adx, c=='o'? 8: (c=='x'? 16:10));
        else if(c == 's') {
                s = (char *)*adx;
                while(c = *s++)
                        UNIX_putchar(c);
        } else if (c == 'D') {
                UNIX_printn(*(long *)adx, 10);
                adx += (sizeof(long) / sizeof(int)) - 1;
        }
        adx++;
        goto loop;
}

/*
 * Print an unsigned integer in base b.
 */
UNIX_printn(n, b)
long n;
{
        register long a;

        if (n<0) {      /* shouldn't happen */
                UNIX_putchar('-');
                n = -n;
        }
        if(a = n/b)
                UNIX_printn(a, b);
        UNIX_putchar("0123456789ABCDEF"[(int)(n%b)]);
}

/*
 * Panic is called on unresolvable
 * fatal errors.
 * It syncs, prints "panic: mesg" and
 * then loops.
 */
UNIX_panic(s)
char *s;
{
        panicstr = s;
        UNIX_update();
        UNIX_printf("panic: %s\n", s);
//        for(;;)
//                UNIX_idle();
//      Um why lock? Let's exit!
        exit(0);
}

/*
 * UNIX_prdev prints a warning message of the
 * form "mesg on dev x/y".
 * x and y are the major and minor parts of
 * the device argument.
 */
UNIX_prdev(str, dev)
char *str;
dev_t dev;
{
//J
//Watcom doesn't like the printf parameters.....
//        UNIX_printf("%s on dev %u/%u\n", str, major(dev), minor(dev));
printf("%s on dev %u/%u\n", str, major(dev), minor(dev));
}

/*
 * deverr prints a diagnostic from
 * a device driver.
 * It prints the device, block number,
 * and an octal word (usually some error
 * status register) passed as argument.
 */
UNIX_deverror(bp, o1, o2)
register struct buf *bp;
{
        UNIX_prdev("err", bp->b_dev);
//J
//Watcom doesn't like the printf parameters.....
//        UNIX_printf("bn=%d er=%x,%x\n", bp->b_blkno, o1,o2);
        printf("bn=%d er=%x,%x\n", bp->b_blkno, o1,o2);
}
