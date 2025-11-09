/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/acct.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/inode.h"
#include "h/proc.h"
#include "h/seg.h"
#include "h/uba.h"
#include "h/map.h"
#include "h/reg.h"
#include "h/mtpr.h"
#include "h/clock.h"
#include "h/page.h"



UNIX_startup(firstaddr)
{
        int unixsize;
        void *res;

        /*
         * Initialize maps
         */


        UNIX_printf("RESTRICTED RIGHTS\n");
        UNIX_printf("USE, DUPLICATION OR DISCLOSURE IS\n");
        UNIX_printf("SUBJECT TO RESTRICTION STATED IN YOUR\n");
        UNIX_printf("CONTRACT WITH WESTERN ELECTRIC COMPANY INC.\n\n");
		/*Add new copyrights*/
        UNIX_printf("Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.\n");
        UNIX_printf("Copyright(C) S.C.O. International Inc. 2002-2003. All rights reserved.\n");

        UNIX_printf("real mem  = %d\n", maxmem*ctob(1) );
		if (maxmem > PHYSPAGES) 
			{	/* more pages than allocated in bit map */
			printf("Warning: more page-frames than allocated in bit map\n");
			printf("   Only %d of %d used. (Increase PHYSPAGES)\n",
			PHYSPAGES,maxmem);
			maxmem = PHYSPAGES;
			}
		unixsize = (firstaddr+USIZE);
		UNIX_meminit(unixsize, maxmem);
		maxmem -= unixsize + 1;
		printf("avail mem = %d\n", maxmem*ctob(1));
		if(MAXMEM < maxmem)
			maxmem = MAXMEM;
		UNIX_mfree(swapmap, nswap, 1);
		swplo--;
		UNIX_printf("UNIX_mbainit() is mostly broken!\n");
		UNIX_mbainit();		/* setup mba mapping regs map */
		UNIX_ubainit();		/* setup uba mapping regs map */
}
