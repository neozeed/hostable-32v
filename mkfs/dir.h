/*
 *	dir.h
 */

struct dir
	{
	ino_t dir_i;		/*	inode of entry	*/
	char dir_nam[DIRSIZ];	/*	name of entry	*/
	};

#define L_DIR   (DIRSIZ+sizeof(ino_t))
