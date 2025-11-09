/*
 *	filesys.h
 *	definition of the superblock
 */

#define	SB_FRBLK	100
#define SB_FRINO        100

struct filsys
	{
	blk_t	s_isize;		/* i-list size in blocks	*/
	blk_t	s_fsize;		/* volume size in blocks	*/
	blk_t 	s_nfree;		/* ptr to in core free blocks	*/
	blk_t	s_free[SB_FRBLK];	/* in core free block list	*/
	ino_t	s_ninode;		/* ptr to in core free inodes   */
	ino_t   s_inode[SB_FRINO];	/* in core free inodes		*/
	char	s_flock;		/* lock to free blk list	*/
	char	s_ilock;		/* lock to free inode list	*/
	char	s_fmod;			/* sb modified flag		*/
	char	s_ronly;		/* mounted read only		*/
	time_t	s_time;			/* time of last modification	*/
	blk_t	s_bfree;		/* number of free blocks	*/
	blk_t	s_ifree;		/* number of free inodes	*/
/*	int	s_pad[48];	   */	/* padding			*/
	};

struct filsys *getfs(dev_t);
int badblock (struct filsys*, blk_t, dev_t);