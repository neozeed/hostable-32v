/* UNIX V6 mkfs utility for MS-DOS					*/
/* Szigeti Szabolcs							*/

#include <stdio.h>
#include "types.h"
#include "param.h"
#include "filsys.h"
#include "ino.h"
#include "dir.h"

#define BLOCK_SIZ	256
#define S_READ		0x02
#define S_WRITE		0x03
#define I_SIZE		16

#define BIOS_DISK	0x13
/*These were int's*/
char b_buf [BLOCK_SIZ];
char s_buf [BLOCK_SIZ];

unsigned int ninode,vol_siz;
unsigned int sectors,heads,unit;
unsigned long first_blk;
FILE *filesystem;
main()
	{
	int i;
	printf("Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.\n");
	filesystem=fopen("unix.fs","wb");
	ask_param();
	mkfs();
/*	for(i=0;i<BLOCK_SIZ;i++)
		b_buf[i]=i; A test...
	blk_io(5,S_WRITE,b_buf);*/
	fclose(filesystem);
	exit(0);
	}

/*     Ask parameters for mkfs						*/

ask_param()
	{
	int i;

	printf ("MaKeFileSystem\n\n");
	printf ("Device physical characteristics\n\n");
/*	printf ("Unit : 0x"); scanf("%x",&unit);
	printf ("Heads: "); scanf ("%u",&heads);
	printf ("Sectors: "); scanf("%u",&sectors);	*/
	printf ("\nFile system logical characteristics\n\n");
	printf ("Size of volume : "); scanf("%u",&vol_siz);
/*	if(unit>0x7f)	{
			printf ("Firt block (hex) : ");
			scanf("%X",&first_blk);
			}
	else	*/
		       first_blk=0;

	printf ("Number of I-nodes : "); scanf("%u",&ninode);
	printf ("\nMkfs in progress...\n");
	for (i=0;i<BLOCK_SIZ;i++)
		b_buf[i]=s_buf[i]=0;
	i=0;
	while(i<vol_siz)
		{
		blk_io(i,5,b_buf);
		i++;
		}
	rewind(filesystem);
	}

/*	make file system						*/

mkfs()
	{
	struct filsys *fp;
	struct inode *ip;
	struct dir   *dp;
	unsigned int sublk;
	unsigned int fstfree;
	unsigned int inoblk;
	unsigned int free;
	int i;

	sublk   = 1;
	fp      = (struct filsys *)s_buf;
	inoblk  = (ninode*I_SIZE+(BLOCK_SIZ-I_SIZE))/BLOCK_SIZ;
	fstfree = sublk+inoblk+1;
	fp->s_isize = inoblk;
	fp->s_fsize = vol_siz;
	fp->s_nfree = 0;
	fp->s_ninode= 0;
	fp->s_flock = 0;
	fp->s_ilock = 0;
	fp->s_fmod  = 0;
	fp->s_ronly = 0;
	fp->s_bfree = 0;
	fp->s_ifree = 0;
	printf("Zeroing Superblock & I-nodes\n");
	for(i=0;i<BLOCK_SIZ;i++)
		b_buf[i]=0;
	for (i=0;i<fstfree;i++)
		{
		blk_io(i,S_WRITE,b_buf);
		printf ("Zeroed: %u\r",i);
		}

	printf ("\nFreeing blocks\n");
	for (free=fstfree+1;free<vol_siz;free++)
		{
		if (fp->s_nfree<=0)
			{
			fp->s_nfree = 1;
			fp->s_free[0]=0;
			}
		if (fp->s_nfree>=100)
			{
			for(i=0;i<BLOCK_SIZ;i++)
				b_buf[i]=0;
			b_buf[0]=fp->s_nfree;
			for(i=1;i<101;i++)
				{
				b_buf[i]=fp->s_free[i-1];
				}
			fp->s_nfree=0;
			blk_io(free,S_WRITE,b_buf);
			}
		fp->s_free[fp->s_nfree++]=free;
	
		fp->s_bfree++;
		printf ("Freed :%u\r",free);
		}
		printf ("\nFreeing I-nodes\n");
	fp->s_ifree=ninode-1;
	for (i=2 ; i <= ( ninode>100 ? 100 : ninode) ; i++)
		fp->s_inode[i-1]=i;
	fp->s_ninode = 	i-1;

	printf ("Writing Super Block\n");
	blk_io(sublk,S_WRITE,s_buf);

	for(i=0;i<BLOCK_SIZ;i++)
		b_buf[i]=0;

	ip=(struct inode*) b_buf;
	ip->i_mode=IFDIR|IEXEC|IREAD|IWRITE|(IEXEC>>3)|(IEXEC>>6)|
			 (IREAD>>3)|(IREAD>>6);
	ip->i_uid=0;
	ip->i_gid=0;
	ip->i_size1=L_DIR*3;
	ip->i_nlink=3;
	ip->i_addr[0]=fstfree;


	printf ("Writing root dir.\n");
	blk_io(sublk+1,S_WRITE,b_buf);


	for(i=0;i<BLOCK_SIZ;i++)
		b_buf[i]=0;
	dp=(struct dir *)b_buf;
	dp[0].dir_i=1;
	dp[0].dir_nam[0]='.';
	dp[1].dir_i=1;
	dp[1].dir_nam[0]='.';
	dp[1].dir_nam[1]='.';

	blk_io(fstfree,S_WRITE,b_buf);
	printf ("\nMKFS done\n");
	}

/*	read/write one block (sector) from/to unit using buf		*/

blk_io(rblk,mode,buf)
unsigned int rblk,mode;
char *buf;
	{
	int j;
	j=0;
	printf("rblk %d mode %d buf[%s] \n",rblk,mode,buf);
	if(mode!=5){
	while(j<BLOCK_SIZ)
		{
		fprintf(filesystem,"%c",buf[j]);
		j++;
		}
	}else	/*fileseek*/
	{
	fseek(filesystem,(rblk*256),SEEK_SET);	
	while(j<BLOCK_SIZ)
		{
		fprintf(filesystem,"%c",buf[j]);
		j++;
		}
	}

/*	unsigned int cyl,sec,hed;
	unsigned long blk;
	static union REGS regs;

	blk=rblk+first_blk;
	cyl = (int)(blk / (heads * sectors));
	sec = (int)(blk % sectors)+1;
	hed = (int)(blk % (heads * sectors)) / sectors;
	regs.h.ah = mode;
	regs.h.dl = unit;
	regs.h.dh = hed;
	regs.h.ch = cyl & 0xff;
	regs.h.cl = (sec & 0x3f) + ((cyl >> 2) & 0xc0);
	regs.h.al = 1;
	regs.x.bx = (int)buf;
	_ES 	  = _DS;
	int86 (BIOS_DISK,&regs,&regs);
	if (regs.x.cflag) 
		error(regs.h.ah);
*/
	}
error(code)
char code;
	{
	fprintf(stderr,"I/O error: %u\n",code);
	exit(1);
	}
