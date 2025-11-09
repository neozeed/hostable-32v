/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/
#include "h/param.h"
#include "h/buf.h"

daddr_t
UNIX_dkblock(bp)
register struct buf *bp;
{
        register int dminor;

        if (((dminor=minor(bp->b_dev))&0100) == 0)
                return(bp->b_blkno);
        dminor >>= 3;
        dminor &= 07;
        dminor++;
        return(bp->b_blkno/dminor);
}

UNIX_dkunit(bp)
register struct buf *bp;
{
        register int dminor;

        dminor = minor(bp->b_dev) >> 3;
        if ((dminor&010) == 0)
                return(dminor);
        dminor &= 07;
        dminor++;
        return(bp->b_blkno%dminor);
}
