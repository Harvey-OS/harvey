#define ERRLEN 64
#define NAMELEN 28
typedef
struct Waitmsg
{
	char	pid[12];	/* of loved one */
	char	time[3*12];	/* of loved one & descendants */
	char	msg[ERRLEN];
} Waitmsg;

#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MMASK	0x0007	/* all bits on */

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */

#define	NCONT	0	/* continue after note */
#define	NDFLT	1	/* terminate after note */

#define CHDIR		0x80000000	/* mode bit for directories */
#define CHAPPEND	0x40000000	/* mode bit for append only files */
#define CHEXCL		0x20000000	/* mode bit for exclusive use files */
#define CHREAD		0x4		/* mode bit for read permission */
#define CHWRITE		0x2		/* mode bit for write permission */
#define CHEXEC		0x1		/* mode bit for execute permission */

#define FORKNSG		(1<<0)		/* arguments to rfork */
#define	FORKEVG		(1<<1)
#define	FORKFDG		(1<<2)
#define	FORKNTG		(1<<3)
#define	FORKPCS		(1<<4)
#define	FORKMEM		(1<<5)
#define	FORKNOW		(1<<6)
#define	FORKCNSG	(1<<10)
#define	FORKCEVG	(1<<11)
#define	FORKCFDG	(1<<12)

extern	int	_ALARM(unsigned long);
extern	int	_BIND(char*, char*, int);
extern	int	_CHDIR(char*);
extern	int	_CLOSE(int);
extern	int	_CREATE(char*, int, unsigned long);
extern	int	_DUP(int, int);
extern	int	_ERRSTR(char*);
extern	int	_EXEC(char*, char*[]);
extern	void	_EXITS(char *);
extern	int	_FSTAT(int, char*);
extern	int	_FWSTAT(int, char*);
extern	int	_MOUNT(int, char*, int, char*, char*);
extern	int	_NOTED(int);
extern	int	_NOTIFY(int(*)(void*, char*));
extern	int	_OPEN(char*, int);
extern	int	_PIPE(int*);
extern	long	_READ(int, void*, long);
extern	int	_REMOVE(char*);
extern	int	_RENDEZVOUS(unsigned long, unsigned long);
extern	int	_RFORK(int);
extern	int	_SEGATTACH(int, char*, void*, unsigned long);
extern	int	_SEGBRK(void*, void*);
extern	int	_SEGDETACH(void*);
extern	int	_SEGFLUSH(void*, unsigned long);
extern	int	_SEGFREE(void*, unsigned long);
extern	long	_SEEK(int, long, int);
extern	int	_SLEEP(long);
extern	int	_STAT(char*, char*);
extern	int	_WAIT(Waitmsg*);
extern	long	_WRITE(int, void*, long);
extern	int	_WSTAT(char*, char*);

extern	int	__open(char *, int, ...);
extern	int	__access(char *, int);
extern	int	__chdir(char *);
extern	int	__creat(char *, int);
extern	int	__link(char *, int);
extern	int	__stat(char *, struct stat *);
extern	int	__unlink(char *);
