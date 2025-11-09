/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/filsys.h"
#include "h/mount.h"
#include "h/uba.h"
#include "h/map.h"
#include "h/proc.h"
#include "h/inode.h"
#include "h/seg.h"
#include "h/conf.h"
#include "h/buf.h"
#include "h/mtpr.h"
#include "h/page.h"
#include "h/clock.h"

//GLOBALS
struct user u;

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *      clear and free user core
 *      turn on clock
 *      hand craft 0th process
 *      call all initialization routines
 *      fork - process 0 to schedule
 *           - process 1 execute bootstrap
 *
 * loop at loc 13 (0xd) in user mode -- /etc/init
 *      cannot be executed.
 */
void missing_init(void);
main(firstaddr)
{
        int i;
		missing_init();
        UNIX_startup(firstaddr);

        /*
         * set up system process
         */

        for(i=0; i<UPAGES; i++) {
                proc[0].p_addr[i] = firstaddr + i;
        }
        proc[0].p_size = UPAGES;
        proc[0].p_stat = SRUN;
        proc[0].p_flag |= SLOAD|SSYS;
        proc[0].p_nice = NZERO;
        u.u_procp = &proc[0];
        u.u_cmask = CMASK;
        UNIX_clkstart();

        /*
         * Initialize devices and
         * set up 'known' i-nodes
         */

        UNIX_cinit();
        UNIX_binit();
        UNIX_iinit();
		
        rootdir = UNIX_iget(rootdev, (ino_t)ROOTINO);
        rootdir->i_flag &= ~ILOCK;
        u.u_cdir = UNIX_iget(rootdev, (ino_t)ROOTINO);
        u.u_cdir->i_flag &= ~ILOCK;
        u.u_rdir = NULL;

        /*
         * make init process
         * enter scheduling loop
         * with system process
         */

        if(UNIX_newproc()) {
                UNIX_expand(btoc(szicode), P0BR);
                u.u_dsize = btoc(szicode);
                UNIX_copyout((caddr_t)icode, (caddr_t)0, szicode);
                /*
                 * Return goes to loc. 0 of user init
                 * code just copied out.
                 */
                //This return thing must be a K&R thing, to goto the UNIX_sched part..
                //return;
        }
		//UNIX_printf("Making Devices\n");
		
        //UNIX_trap(14,"/dev",0140755,0,0);
        /*sys_x(9,"/dev","/dev/.",0,0);
        sys_x(9,"/.","/dev/..",0,0);
        sys_x(14,"/dev/tty0",0120666,0,0);
        sys_x(36,0,0,0,0);
        sys_x(1,1,0,0,0);*/
		{
			int f;
		f=UNIX_creat("test", 0666);
		}

        UNIX_sched();
        UNIX_panic("main.c sched()\n");
}

/*
 * iinit is called once (from main)
 * very early in initialization.
 * It reads the root's super block
 * and initializes the current date
 * from the last modified date.
 *
 * panic: iinit -- cannot read the super
 * block. Usually because of an IO error.
 */
UNIX_iinit()
{
        register struct buf *cp, *bp;
        register struct filsys *fp;
        register unsigned i , j ;

        (*bdevsw[major(rootdev)].d_open)(rootdev, 1);
        bp = UNIX_bread(rootdev, SUPERB);
        cp = UNIX_geteblk();
        if(u.u_error)
                UNIX_panic("iinit");
        UNIX_bcopy(bp->b_un.b_addr, cp->b_un.b_addr, sizeof(struct filsys));
        UNIX_brelse(bp);
        mount[0].m_bufp = cp;
        mount[0].m_dev = rootdev;
        fp = cp->b_un.b_filsys;
        fp->s_flock = 0;
        fp->s_ilock = 0;
        fp->s_ronly = 0;
        /* on boot, read VAX TODR register (GMT 10 ms.
        *       clicks into current year) and set software time
        *       in 'int time' (GMT seconds since year YRREF)
        */
        for (i = 0 , j = YRREF ; j < YRCURR ; j++)
                i += (SECYR + (j%4?0:SECDAY)) ;
        time = UNIX_udiv(UNIX_mfpr(TODR),100) + i ;
}

/*
 * This is the set of buffers proper, whose heads
 * were declared in buf.h.  There can exist buffer
 * headers not pointing here that are used purely
 * as arguments to the I/O routines to describe
 * I/O to be done-- e.g. swbuf for
 * swapping.
 */
//char    buffers[NBUF][BSIZE+BSLOP];
char buffers[1024][BSIZE+BSLOP+512];

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
UNIX_binit()
{
        register struct buf *bp;
        register struct buf *dp;
        register int i;
        struct bdevsw *bdp;

        bfreelist.b_forw = bfreelist.b_back =
            bfreelist.av_forw = bfreelist.av_back = &bfreelist;
        for (i=0; i<NBUF; i++) {
                bp = &buf[i];
                bp->b_dev = NODEV;
                bp->b_un.b_addr = buffers[i];
                bp->b_back = &bfreelist;
                bp->b_forw = bfreelist.b_forw;
                bfreelist.b_forw->b_back = bp;
                bfreelist.b_forw = bp;
                bp->b_flags = B_BUSY;
                UNIX_brelse(bp);
        }
        for (bdp = bdevsw; bdp->d_open; bdp++) {
                dp = bdp->d_tab;
                if(dp) {
                        dp->b_forw = dp;
                        dp->b_back = dp;
                }
                nblkdev++;
        }
}
