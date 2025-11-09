/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

#include "h/param.h"
#include "h/systm.h"
#include "h/callo.h"
#include "h/seg.h"
#include "h/dir.h"
#include "h/user.h"
#include "h/proc.h"
#include "h/reg.h"
#include "h/psl.h"


//SOMEboxes need size_t defined
typedef unsigned long size_t;




/*Set priority levels.. mostly usless under a hosted enviroment..*/
void spl7(void){}
void spl6(void){}
void spl5(void){}
void spl4(void){}
void spl3(void){}
void spl2(void){}
void spl1(void){}
void spl0(void){}

void splx(void){}

/*The fubyte kernel service fetches, or retrieves, a byte of data from the specified address in user memory. It is provided so that system calls and device heads can safely access user data. The fubyte service ensures that the user has the appropriate authority to:

Access the data. 
Protect the operating system from paging I/O errors on user data.
The fubyte service should be called only while executing in kernel mode in the user process.
*/
int UNIX_fubyte(unsigned char *uaddr){return(&uaddr);}
int UNIX_fuword(unsigned int *uaddr){return(&uaddr);}
void UNIX_suword(void){UNIX_panic("suword");}
void UNIX_subyte(void){UNIX_panic("subyte");}
void UNIX_fuibyte(void){UNIX_panic("fuibyte");}
void UNIX_fuiword(void){UNIX_panic("fuiword");}
void UNIX_suiword(void){UNIX_panic("suiword");}
void UNIX_suibyte(void){UNIX_panic("suibyte");}

/* Unsigned Divide..
LOCORE.S
     (int) i = udiv( (int)dvdnd , (int) divis)
*/
int UNIX_udiv(int dvdnd,int divis){return(dvdnd/divis);}

void UNIX_mtpr(void){UNIX_panic("mtpr");}
void UNIX_ttioccom(void){UNIX_panic("ttioccom");}

/*Isn't save to save register state between task switches?*/
void UNIX_save(void){}
/*Add user process?*/
void UNIX_addupc(void){UNIX_panic("addupc");}

/*U belive these are to copy pages between kernel and user memory process*/
void UNIX_copyin(void){UNIX_panic("copyin");}

/*
copyout -- copy data from a driver buffer to a user buffer 
int copyout(void * driverbuf, void * userbuf, size_t count);
*/
void UNIX_copyout(void * driverbuf, void * userbuf, size_t count)
{
	memcpy(userbuf,driverbuf,count);
}

/*I think these are for port IO*/
void UNIX_ptaccess(void){UNIX_panic("ptaccess");}

/*
Memory Maps
I have no idea how big these arrays are..
*/
int swap2utl[1024*1024];    /*I have no idea how big these arrays are..*/
int Swap2map[1024*1024];    
int Swapmap[1024];
int swaputl[1024];
int Xswapmap[1024];
int xallutl[1024];
int xccdutl[1024];
int Xallmap[1024];
int xswap2utl[1024];
int Xswap2map[1024];
int xswaputl[1024];
int Xccdmap[1024];
int Umap[1024*1024];
int mmap[1024*1024]; 
char vmmap[1024*1024];
int Sysmap[1024*1024];
int Mbamap[1024];
int mbautl[1024];
int	icode[1024*1024];//Something to do with INODES I guess.....





void UNIX_clearseg(void){UNIX_panic("clearseg");}

void dump_buf(char *buf,size_t count)
{
	int i;
	i=0;
	while(i<count)
	{printf("%X",buf[i]);i++;}
	printf("\n");
}
/*bcopy I think is like memcpy
void *dest,const void *src,size_t count 
bcopy(from,to,count)
*/
void UNIX_bcopy(void *src,const void *dest,size_t count)
{printf("UNIX_bcopy copying %d bytes\n",count);
	memcpy(dest,src,count);}

void UNIX_chksize(void){UNIX_panic("chksize");}
void UNIX_mptpr(void){}
void UNIX_mfpr(void){}

/*Change protection?*/
void UNIX_chgprot(void){}

/*I think this is the 'idle' task*/
void UNIX_idle(void){}
/*something to do with waking up?*/
void UNIX_resume(void){}

void UNIX_useracc(void){UNIX_panic("useracc");}
void UNIX_kernacc(void){UNIX_panic("kernacc");}

void UNIX_sysphys(void){UNIX_panic("sysphys");}
//void UNIX_szicode(void){}
int     szicode; 

void UNIX_sendsig(void){}
void UNIX_procdup(void){}

void UNIX_ptexpand(void){UNIX_panic("ptexpand");}

/*clock stuff*/
void UNIX_clkstart(void){}
void UNIX_clkreld(void){}
void UNIX_waitloc(void){}
void UNIX_ewaitloc(void){}
caddr_t waitloc, ewaitloc;



/*WIN32junk to fake out the win32 build...... you don't need to uncomment this, this is just
to make sure it's clean (without using standard libs these functions are part of the runtime)
void _RTC_CheckEsp(void){}
void _RTC_Shutdown(void){}
void _RTC_InitBase(void){}
void _RTC_UninitUse(void){}
void _RTC_CheckStackVars8(void){}
void mainCRTStartup(void){}*/

void missing_init(void)
{
memset(swap2utl,0x0,sizeof(swap2utl));
memset(Swap2map,0x0,sizeof(Swap2map));
memset(Swapmap,0x0,sizeof(Swapmap));
memset(swaputl,0x0,sizeof(swaputl));
memset(Xswapmap,0x0,sizeof(Xswapmap));
memset(xallutl,0x0,sizeof(xallutl));
memset(xccdutl,0x0,sizeof(xccdutl));
memset(Xallmap,0x0,sizeof(Xallmap));
memset(xswap2utl,0x0,sizeof(xswap2utl));
memset(Xswap2map,0x0,sizeof(Xswap2map));
memset(xswaputl,0x0,sizeof(xswaputl));
memset(Xccdmap,0x0,sizeof(Xccdmap));
memset(Umap,0x0,sizeof(Umap));
memset(mmap,0x0,sizeof(mmap)); 
memset(vmmap,0x0,sizeof(vmmap));
memset(Sysmap,0x0,sizeof(Sysmap));
memset(Mbamap,0x0,sizeof(Mbamap));
memset(icode,0x0,sizeof(icode));
memset(mbautl,0x0,sizeof(mbautl));
maxmem=1024; //Just to be safe.
}
