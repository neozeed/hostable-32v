/*
 *	ino.h
 *      on disk inode structure
 */

struct  inode
	{
	int     i_mode;		/* see below			*/
	u_char  i_nlink;	/* nuber of links to inode	*/
	uid_t   i_uid;		/* owner uid			*/
	gid_t   i_gid;		/* owner gid			*/
	byte    i_size0;	/* bits 23..16  of length	*/
	word	i_size1;        /* bits 15..0   of length	*/
	blk_t   i_addr[8];	/* blocks of the file		*/
	time_t  i_atime;	/* time of last access		*/
	time_t  i_mtime;	/* time of last modification	*/
	};

/* modes */

#define IALLOC  0100000		/* allocated			*/
#define IFMT     060000         /* mask for type		*/
#define  IFDIR   040000		/* inode is directory		*/
#define  IFCHR   020000		/* inode is char special	*/
#define  IFBLK   060000		/* inode is block special	*/
#define ILARG    010000		/* file is indirect 		*/
#define ISUID     04000		/* set owner id on exec		*/
#define ISGID     02000		/* set group id on exec		*/
#define ISVTX     01000		/* keep text on swap		*/
#define IREAD      0400		/* can be read			*/
#define IWRITE     0200		/* can be written		*/
#define IEXEC      0100		/* can be executed		*/

