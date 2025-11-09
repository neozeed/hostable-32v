/*
 *	param.h
 *	system parameters
 */

#define CPU	i286

#define NBUF	20 	/* Number of buffers in buffer cache	*/
#define NINODE	100	/* Max in core inodes			*/
#define NFILE	100	/* Max open files			*/
#define	NPROC	50	/* Max processes		        */
#define NEXEC	3	/* Max processes doing exec		*/
#define NMOUNT	5	/* Max mountable filesystems		*/
#define NOFILE	15	/* Max open file per process		*/
#define CANBSIZ	80	/* Size of the canonical buffer		*/
#define CMAPSIZ	100	/* Max core map entries			*/
#define SMAPSIZ 100	/* Max swap map entries			*/
#define NCLIST	100	/* Max number clists			*/
#define NTEXT	40	/* Max pure text			*/
#define NTOUT	30	/* Maximum callout entries		*/
#define KSSIZ	256	/* Kernel stack size per process	*/
#define	HZ	50      /* Frequency of the system clock	*/


#define MAXPRI 128
#define MINPRI -128

/* priorities	*/

#define	PSWP	-100	/* swapper	*/
#define PINOD	-90	/* inode wait	*/
#define	PRIBIO	-50	/* block wait	*/
#define PPIPE	1	/* pipe wait	*/
#define PWAIT	40
#define PSLEP	90
#define PUSER	100     /* user proc	*/

/* signals	*/

#define NSIG	20
#define		SIGHUP	1	/* hangup		*/
#define         SIGINT	2	/* interrupt		*/
#define		SIGQIT	3	/* quit			*/
#define		SIGINS	4	/* illegal instruction	*/
#define		SIGTRC	5	/* trace		*/
#define		SIGIOT	6	/* iot			*/
#define		SIGFPT	8	/* floating coproc err  */
#define		SIGKIL  9	/* kill			*/
#define		SIGBUS	10	/* bus error		*/
#define		SIGSEG	11	/* segment violation #13*/
#define		SIGSYS	12	/* sys			*/
#define		SIGPIPE	13	/* broken pipe		*/
#define 	SIGBND	14	/* bound exceeded	*/
#define 	SIGTERM	15	/* software terminate	*/
#define		SIGOVF	16	/* overflow trap	*/
#define		SIG17	17
#define		SIG18	18
#define		SIG19	19
#define		SIG20	20

/* fundamental constants	*/

#define NODEV   (-1)
#define NULL	0
#define	ROOTINO	1
#define DIRSIZ	14


#define lock()		asm cli
#define enable()	asm sti

#define	minor(x)	((x)&0xff)	/* minor device szam		*/
#define major(x)	((x)>>8)	/* major device szam		*/
#define makedev(ma,mi)	(((ma)<<8)+(mi)&0xff)

#define min(a,b)	((a)<(b)?(a):(b))
#define max(a,b)	((a)<(b)?(b):(a))


