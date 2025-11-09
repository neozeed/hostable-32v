/*
 * Random set of variables
 * used by more than one
 * routine.
 */
char	canonb[CANBSIZ];	/* buffer for erase and kill (#@) */
struct inode *rootdir;		/* pointer to inode of root directory */
struct proc *runq;		/* head of linked list of running processes */
int	cputype;		/* type of cpu =40, 45, or 70 */
int	lbolt;			/* time of day in 60th not in time */
time_t	time;			/* time in sec from 1970 */

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in binit/bio.c by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */
int	nblkdev;

/*
 * Number of character switch entries.
 * Set by cinit/tty.c
 */
int	nchrdev;

int	mpid;			/* generic for unique process id's */
char	runin;			/* scheduling flag */
char	runout;			/* scheduling flag */
char	runrun;			/* scheduling flag */
char	curpri;			/* more scheduling */
int	maxmem;			/* actual max memory per process */
int	physmem;			/* physical memory on this CPU */
int	freemem;			/* remaining blocks of free memory */
daddr_t	swplo;			/* block number of swap space */
int	nswap;			/* size of swap space */
int	updlock;		/* lock for sync */
daddr_t	rablock;		/* block to be read ahead */
char	msgbuf[MSGBUFS];	/* saved "printf" characters */
int	intstack[512];		/* stack for interrupts */
dev_t	rootdev;		/* device of the root */
dev_t	swapdev;		/* swapping device */
dev_t	pipedev;		/* pipe device */
extern  int     icode[];   /*      user init code */  /*Why is this commented out?*/
extern	int	szicode;	/* its size */

dev_t UNIX_getmdev();
daddr_t	UNIX_bmap();
struct inode *UNIX_ialloc();
struct inode *UNIX_iget();
struct inode *UNIX_owner();
struct inode *UNIX_maknode();
struct inode *UNIX_namei();
struct buf *alloc();
struct buf *UNIX_getblk();
struct buf *UNIX_geteblk();
struct buf *bread();
struct buf *breada();
struct filsys *UNIX_getfs();
struct file *UNIX_getf();
struct file *UNIX_falloc();
int	UNIX_uchar();
caddr_t	realaddr();
caddr_t	checkio();
/*
 * Instrumentation
 */
int	dk_busy;
long	dk_time[32];
long	dk_numb[3];
long	dk_wds[3];
long	tk_nin;
long	tk_nout;

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	char	sy_narg;		/* total number of arguments */
	char	sy_nrarg;		/* number of args in registers */
	int	(*sy_call)();		/* handler */
} sysent[];
