/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

/*
 *      Memory special file
 *      minor device 0 is physical memory
 *      minor device 1 is kernel memory 
 *      minor device 2 is EOF/RATHOLE
 */

#include "h/param.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/conf.h"
#include "h/buf.h"
#include "h/systm.h"
#include "h/page.h"
#include "h/mtpr.h"

UNIX_mmread(dev)
{
        register char *p;
        register c;
        extern int mmap[];
        extern char vmmap[];

        if(minor(dev) == 2)
                return;
        if( minor(dev) == 1 ) {
                if( UNIX_kernacc(u.u_offset, u.u_count, B_READ)
            && !UNIX_copyout(u.u_offset, u.u_base, u.u_count) ) {
                c = u.u_count;
                u.u_count = 0;
                u.u_base += c;
                u.u_offset += c;
        } else 
                u.u_error = EFAULT;
        return;
        }

        if( minor(dev) == 0) {
                while (u.u_count > 0 && u.u_error == 0) {
                        c = (int) u.u_offset >> 9;
                        if( c < 0 || c >= MAXMEM ) {
                                u.u_error = EFAULT;
                                break;
                        }
                        *mmap = c | (PG_V | PG_KR);
                        UNIX_mtpr(TBIS, vmmap);
                        UNIX_copyout(vmmap+((int)u.u_offset & 0x1ff), u.u_base, c=UNIX_min(512-((int)u.u_offset & 0x1ff), u.u_count));
                        u.u_count -= c;
                        u.u_base += c;
                        u.u_offset += c;
                }
        return;
        }
 
        if (minor(dev) == 3) { /* UNIBUS access */
                if ((!UNIX_kernacc(u.u_offset,u.u_count,B_READ)) ||
                (!UNIX_useracc(u.u_base,u.u_count,B_READ)) ||
                UNIcpy((short*)u.u_offset,(short *)u.u_base,u.u_count,B_READ))
                  u.u_error = EFAULT;
                else {
                        u.u_offset += u.u_count;
                        u.u_base += u.u_count;
                        u.u_count = 0;
                }
                return;
        }
}

UNIX_mmwrite(dev)
{
        register char *p;
        register c;

        if(minor(dev) == 2) {
                c = u.u_count;
                u.u_count = 0;
                u.u_offset += c;
                return;
        }
        /*  kernel virt mem */
        if (minor(dev) == 1) {
                if ((!UNIX_kernacc(u.u_offset,u.u_count,B_WRITE)) || UNIX_copyin(u.u_base,u.u_offset,u.u_count))
                        u.u_error = EFAULT;
                else {
                        u.u_offset += u.u_count;
                        u.u_base += u.u_count;
                        u.u_count = 0 ;
                }
                return;
        }
 
        if (minor(dev) == 3) { /* UNIBUS access */
                if ((!UNIX_kernacc(u.u_offset,u.u_count,B_WRITE)) ||
                (!UNIX_useracc(u.u_base,u.u_count,B_WRITE)) ||
                UNIcpy((short *)u.u_offset,(short *)u.u_base,u.u_count,B_WRITE))
                        u.u_error = EFAULT;
                else {
                        u.u_offset += u.u_count;
                        u.u_base += u.u_count;
                        u.u_count = 0;
                }
                return;
        }
}

/*
 *  UNIBUS Address Space <-->  User Space transfer
 */
UNIcpy(uniadd,usradd,bknt,direct)
short *uniadd , *usradd;
{
        register short *from , *to;
        register int i;
 
        if (direct == B_READ) {
                from = uniadd;
                to = usradd ;
        } else {
                if (direct == B_WRITE) {
                        from = usradd;
                        to = uniadd ;
                }
        }
 
        for (i = (bknt>>1) ; i>0 ; i--)
                (*to++) = (*from++);
        return(0);
}
