/*
 * Lame ANSI C 'file system' disk driver
 * Check out http://www.tom-yam.or.jp/2238/ref/iosys.pdf
 * For some clue on how the IO system on UNIX works.. 
 * Block IO is a little more complicated that I thought, but 
 * I guess that's why it's so sophisticated....
 */

#include "h/param.h"
#include "h/uba.h"
#include "h/systm.h"
#include "h/buf.h"
#include "h/conf.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/map.h"
#include "h/mba.h"

#define DK_N    0

struct  device
{
        int     hpcs1;          /* control and Status register 1 */
        int     hpds;           /* Drive Status */
        int     hper1;          /* Error register 1 */
        int     hpmr;           /* Maintenance */ 
        int     hpas;           /* Attention Summary */
        int     hpda;           /* Desired address register */
        int     hpdt;           /* Drive type */
        int     hpla;           /* Look ahead */
        int     hpsn;           /* serial number */
        int     hpof;           /* Offset register */
        int     hpdc;           /* Desired Cylinder address register */
        int     hpcc;           /* Current Cylinder */
        int     hper2;          /* Error register 2 */
        int     hper3;          /* Error register 3 */
        int     hpec1;          /* Burst error bit position */
        int     hpec2;          /* Burst error bit pattern */
};

#define HPADDR  ((struct device *)(MBA0 + MBA_ERB))
#define NHP     2
#define RP      022
#define RM      024
#define NSECT   22
#define NTRAC   19
#define NRMSECT 32
#define NRMTRAC 5
#define SDIST   2
#define RDIST   6

#define P400    020
#define M400    0220
#define P800    040
#define M800    0240
#define P1200   060
#define M1200   0260

struct  buf     ansitab;
struct  buf     ransibuf;
struct  buf     ansiutab[NHP];
char    hp_type[NHP];   /* drive type */

#define GO      01
#define WCOM    060	//BS flags stolen from older drivers.
#define RCOM    070

#define b_cylin b_resid

//#include "D:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\include\stdio.h" //for file operations!
#include <stdio.h>

FILE *filesystem;
void dump_buf(char *buf,size_t count);
 
daddr_t UNIX_dkblock();
 /*was hpstrategy*/
UNIX_ansistrategy(bp)
register struct buf *bp;
{
	UNIX_printf("ansi strategy\n");
	if(bp->b_flags&B_PHYS)
		UNIX_printf("UNIX_mapalloc(bp);(taken from rk driver (pdp11v7))\n");
	bp->av_forw = (struct buf *)NULL;
	spl5();
	if(ansitab.b_actf == NULL)
		ansitab.b_actf = bp;
	else
		ansitab.b_actl->av_forw = bp;
	ansitab.b_actl = bp;
	if(ansitab.b_active == NULL)
		ansistart();
}
ansistart()
{
register struct buf *bp;
register com;
daddr_t bn;
int dn, cn, sn;
	UNIX_printf("ansistart\n");

	if ((bp = ansitab.b_actf) == NULL)
	{
		UNIX_printf("ansitab.b_actf==NULL\n");
		return;
	}
	ansitab.b_active++;
	bn = bp->b_blkno;
	dn = minor(bp->b_dev);
	cn = bn/12;
	sn = bn%12;
	#define	RKADDR	((struct device *)0177400)
	#define	IENABLE	0100
	//RKADDR->hpda = (dn<<13) | (cn<<4) | sn;
	//RKADDR->hpds = bp->b_un.b_addr;			//Is this the actual IO?
	//RKADDR->hpdc = -(bp->b_bcount>>1);
	com = ((bp->b_xmem&3) << 4) | IENABLE | GO;
	if(bp->b_flags & B_READ)
		com |= RCOM; else		//read command
		com |= WCOM;			//write command
	/*Some debug info*/
	printf("bp = %o\n", bp);
	printf("b_flags = %d\n", bp->b_flags, "\010\020RAMREMAP\017UBAREMAP\
		\016LOCKED\015BAD\014INVAL\013TAPE\012DELWRI\011ASYNC\010AGE\
		\007WANTED\006MAP\005PHYS\004BUSY\003ERROR\002DONE\001READ");
	printf("b_forw = %o, b_back = %o, av_forw = %o, av_back = %o\n",
		bp->b_forw, bp->b_back, bp->av_forw, bp->av_back);
	printf("b_bcount = %u, b_error = %o\n", bp->b_bcount, bp->b_error); 
	printf("b_xmem = %o, b_addr = %o, physaddr = %o\n",
		bp->b_xmem, bp->b_un.b_addr,
		(long)bp->b_xmem<<16 | (long)bp->b_un.b_addr);
	printf("b_blkno = %ld\n", (long)bp->b_blkno);


	//RKADDR->hpas = com;
	dk_busy |= 1<<DK_N;
	dk_numb[DK_N] += 1;
	com = bp->b_bcount>>6;
	dk_wds[DK_N] += com;
	//Because we are emulating, I'll manally trigger an 'interrupt'
	UNIX_ansiintr();
}
/*was hpustart*/
UNIX_ansiustart(unit)
register unit;
{
	//This function should do more, but it's safe to say it's busted.
	printf("ansiustart\n");
	
}
/*hpstart*/
UNIX_ansistart(struct buf *bp)
{
	//Check to see that ansitab.d_actf is populated...
	//This function should do more, but it's safe to say it's busted.
	printf("ansistart\n");
}

/*hpintr*/
/*
I belive that this function should get called once IO has completed.
*/
//UNIX_ansiintr(mbastat, as)
UNIX_ansiintr()
{
	register struct buf *bp;
	UNIX_printf("UNIX_ansiintr()\n");
	if (ansitab.b_active == NULL)
		return;
	dk_busy &= ~(1<<DK_N);
	bp = ansitab.b_actf;
	ansitab.b_active = NULL;
	/*Do some error status checking, pointless in emulation though*/
	ansitab.b_errcnt = 0;
	ansitab.b_actf = bp->av_forw;
	bp->b_resid = 0;
	UNIX_iodone(bp);
	ansistart();
}

UNIX_ansiread(dev)
{
printf("ansiread\n");
        UNIX_physio(UNIX_ansistrategy, &ransibuf, dev, B_READ);
}

UNIX_ansiwrite(dev)
{
printf("ansiwrite\n");
        UNIX_physio(UNIX_ansistrategy, &ransibuf, dev, B_WRITE);
}



UNIX_ansiopen()
{
	filesystem=fopen("unix.fs","rb");
		if(filesystem==NULL)
			UNIX_panic("Filesystem file unix.fs not found!\n");
//	printf("ansiopen\n");
	UNIX_printf("ansifilesystem() read write? block device driver.\n");
	ansitab.b_active == 0;
}
UNIX_ansiclose()
{
	fclose(filesystem);
	printf("ansiclose\n");
}
