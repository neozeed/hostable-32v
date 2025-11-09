/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/
#include "h/param.h"
#include "h/systm.h"
#include "h/buf.h"
#include "h/tty.h"
#include "h/conf.h"
#include "h/proc.h"
#include "h/text.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/file.h"
#include "h/inode.h"
#include "h/mx.h"
#include "h/acct.h"
#include "h/mba.h"

int     nulldev();
int     nodev();
int     random();

int     UNIX_hpstrategy(),UNIX_hpread(),UNIX_hpwrite(),UNIX_hpintr();
int     UNIX_ansistrategy(),UNIX_ansiread(),UNIX_ansiwrite(),UNIX_ansiintr(),UNIX_ansiopen(),UNIX_ansiclose();
struct  buf     hptab;
 
int     UNIX_htopen(),UNIX_htclose(),UNIX_htstrategy(),UNIX_htread(),UNIX_htwrite();
struct  buf     httab;
/*THIS IS THE ORIGINAL bdevsw ... I need to make new devices!!!!!!!!!!!!!!!!!!!!!!!!!*/
struct bdevsw   ORIGbdevsw[] =
{
/* 0 */ nulldev,        nulldev,        UNIX_hpstrategy,        &hptab,
/* 1 */ UNIX_htopen,            UNIX_htclose,   UNIX_htstrategy,        &httab,
        0,
};
/*THIS NEEDS FIXING FOR NATIVE FUNCTIONS BIGTIME! BUT WE CAN SEE THE PANIC FOR DEVTAB!*/
struct bdevsw   bdevsw[] =
{/* 0 */        UNIX_ansiopen,        UNIX_ansiclose,UNIX_ansistrategy,&hptab,
/* 1 */ nulldev,        nulldev,0,0,
0,
};

int     UNIX_consopen(),UNIX_consclose(),UNIX_consread(),UNIX_conswrite(),UNIX_consioctl();
int     UNIX_dzopen(),UNIX_dzclose(),UNIX_dzread(),UNIX_dzwrite(),UNIX_dzioctl();
struct  tty     dz_tty[];
int     UNIX_syopen(),UNIX_syread(),UNIX_sywrite(),UNIX_sysioctl();
int     UNIX_mmread(),UNIX_mmwrite();
int     UNIX_mxopen(),UNIX_mxclose(),UNIX_mxread(),UNIX_mxwrite(),UNIX_mxioctl();
int     UNIX_mcread(),UNIX_mcwrite();
 
struct cdevsw   cdevsw[] =
{
/* 0 */ UNIX_consopen, UNIX_consclose, UNIX_consread, UNIX_conswrite, UNIX_consioctl, 0,
/* 1 */ UNIX_dzopen, UNIX_dzclose, UNIX_dzread, UNIX_dzwrite, UNIX_dzioctl, dz_tty,
/* 2 */ UNIX_syopen,            nulldev,        UNIX_syread,    UNIX_sywrite,   UNIX_sysioctl, 0,
/* 3 */ nulldev,        nulldev,        UNIX_mmread,    UNIX_mmwrite,   nodev, 0,
/* 4 */ nulldev,        nulldev,        UNIX_ansiread,    UNIX_ansiwrite,   nodev, 0,
/* 4  nulldev,        nulldev,        UNIX_hpread,    UNIX_hpwrite,   nodev, 0,*/
/* 5 */ UNIX_htopen,            UNIX_htclose,   UNIX_htread,    UNIX_htwrite,   nodev, 0,
/* 6 */ UNIX_mxopen, UNIX_mxclose, UNIX_mxread, UNIX_mxwrite, UNIX_mxioctl, 0,
        0,
};

int UNIX_ttyopen(),UNIX_ttyclose(),UNIX_ttread(),UNIX_ttwrite();
int UNIX_ttyinput(),UNIX_ttstart() ;
 
struct linesw linesw[] =
{
/* 0 */ UNIX_ttyopen, nulldev, UNIX_ttread, UNIX_ttwrite, nodev, UNIX_ttyinput, UNIX_ttstart,
/* 1 */ UNIX_mxopen, UNIX_mxclose, UNIX_mcread, UNIX_mcwrite, UNIX_mxioctl, nulldev, nulldev,
        0
};
 
int nldisp = 1;
dev_t   rootdev = makedev(0, 0);
dev_t   swapdev = makedev(0, 1);
dev_t   pipedev = makedev(0, 0);
daddr_t swplo = 0;
int nswap = 8778;
 
struct  buf     buf[NBUF];
struct  file    file[NFILE];
struct  inode   inode[NINODE];
struct  text    text[NTEXT];
struct  proc    proc[NPROC];
struct  buf     bfreelist;
struct  acct    acctbuf;
struct  inode   *acctp;
 
int *usrstack = (int *)USRSTACK;
/*tbl*/ int mem_no = 3;         /* major device number of memory special file */


extern int Sysmap[];
extern struct user u;

int mbanum[] = {        /* mba number of major device */
        0,              /* disk */
        1,              /* tape */
        9999999,        /* unused */
        9999999,        /* unused */
        0,              /* disk, raw */
        1,              /* tape, raw */
        };

int *mbaloc[] = { /* virtual location of mba */
        (int *)MBA0,
        (int *)MBA1,
        };
