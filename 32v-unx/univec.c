/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

# include "h/param.h"
/* controller no.'s for bits 27-31 of ISR addr */
# define DEV_1  0x08000000
# define DEV_2  0x10000000
/* Interrupt Service Routine (ISR) addresses */
extern UNIX_ubastray() ;
extern  UNIX_dzrint() , UNIX_dzxint() ;
 
int *UNIvec[BSIZE/NBPW] = {
/* 0x0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x10 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x20 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x30 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x40 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x50 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x60 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x70 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x80 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x90 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0xa0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0xb0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0xc0 */
        (int *)UNIX_dzrint,     /* DZ-11 # 0 */
        (int *)UNIX_dzxint,
/*        (int *)((int)UNIX_dzrint+DEV_1),        /*  DZ-11  # 1 */
/*        (int *)((int)UNIX_dzxint+DEV_1),
/* 0xd0 */
        (int *)UNIX_ubastray, /* DR-11B, VAX-11/45 link */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0xe0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0xf0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x100 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x110 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x120 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x130 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x140 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x150 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x160 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x170 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x180 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x190 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x1a0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x1b0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x1c0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x1d0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x1e0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
/* 0x1f0 */
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        (int *)UNIX_ubastray,
        } ;
 
UNIX_ubastray()
{
UNIX_printf("stray UBA interrupt\n") ;
}
