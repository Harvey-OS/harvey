typedef
struct Waitmsg
{
	int pid;	/* of loved one */
	unsigned long time[3];	/* of loved one & descendants */
	char	*msg;
} Waitmsg;

#define	STATMAX	65535U	/* max length of machine-independent stat structure */
#define	DIRMAX	(sizeof(Dir)+STATMAX)	/* max length of Dir structure */
#define	ERRMAX	128	/* max length of error string */

#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MCACHE	0x0010	/* cache some data */
#define	MMASK	0x0007	/* all bits on */

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */
#define	OEXCL	0x1000	/* or'ed in, exclusive use (create only) */

#define	AEXIST	0	/* accessible: exists */
#define	AEXEC	1	/* execute access */
#define	AWRITE	2	/* write access */
#define	AREAD	4	/* read access */

/* Segattch */
#define	SG_RONLY	0040	/* read only */
#define	SG_CEXEC	0100	/* detach on exec */

#define	NCONT	0	/* continue after note */
#define	NDFLT	1	/* terminate after note */
#define	NSAVE	2	/* clear note but hold state */
#define	NRSTR	3	/* restore saved state */

/* bits in Qid.type */
#define QTDIR		0x80		/* type bit for directories */
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTFILE		0x00		/* plain file */

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMREAD		0x4		/* mode bit for read permission */
#define DMWRITE		0x2		/* mode bit for write permission */
#define DMEXEC		0x1		/* mode bit for execute permission */

/* rfork */
enum
{
	RFNAMEG		= (1<<0),
	RFENVG		= (1<<1),
	RFFDG		= (1<<2),
	RFNOTEG		= (1<<3),
	RFPROC		= (1<<4),
	RFMEM		= (1<<5),
	RFNOWAIT	= (1<<6),
	RFCNAMEG	= (1<<10),
	RFCENVG		= (1<<11),
	RFCFDG		= (1<<12),
	RFREND		= (1<<13),
	RFNOMNT		= (1<<14)
};

extern	int	_AWAIT(char*, int);
extern	int	_ALARM(unsigned long);
extern	int	_BIND(const char*, const char*, int);
extern	int	_CHDIR(const char*);
extern	int	_CLOSE(int);
extern	int	_CREATE(char*, int, unsigned long);
extern	int	_DUP(int, int);
extern	int	_ERRSTR(char*, unsigned int);
extern	int	_EXEC(char*, char*[]);
extern	void	_EXITS(char *);
extern	int	_FD2PATH(int, char*, int);
extern	int	_FAUTH(int, char*);
extern	int	_FSESSION(int, char*, int);
extern	int	_FSTAT(int, unsigned char*, int);
extern	int	_FWSTAT(int, unsigned char*, int);
extern	int	_MOUNT(int, int, const char*, int, const char*);
extern	int	_NOTED(int);
extern	int	_NOTIFY(int(*)(void*, char*));
extern	int	_OPEN(const char*, int);
extern	int	_PIPE(int*);
extern	long	_PREAD(int, void*, long, long long);
extern	long	_PWRITE(int, void*, long, long long);
extern	long	_READ(int, void*, long);
extern	int	_REMOVE(const char*);
extern	int	_RENDEZVOUS(unsigned long, unsigned long);
extern	int	_RFORK(int);
extern	int	_SEGATTACH(int, char*, void*, unsigned long);
extern	int	_SEGBRK(void*, void*);
extern	int	_SEGDETACH(void*);
extern	int	_SEGFLUSH(void*, unsigned long);
extern	int	_SEGFREE(void*, unsigned long);
extern	long long	_SEEK(int, long long, int);
extern	int	_SEMACQUIRE(long*, int);
extern	long	_SEMRELEASE(long*, long);
extern	int	_SLEEP(long);
extern	int	_STAT(const char*, unsigned char*, int);
extern	Waitmsg*	_WAIT(void);
extern	int	_TSEMACQUIRE(long*, unsigned long);
extern	long	_WRITE(int, const void*, long);
extern	int	_WSTAT(const char*, unsigned char*, int);

extern	int	__open(char *, int, ...);
extern	int	__access(char *, int);
extern	int	__chdir(char *);
extern	int	__creat(char *, int);
extern	int	__link(char *, int);
extern	int	__stat(char *, struct stat *);
extern	int	__unlink(char *);

/*
 * atomic
 */
long	ainc(long*);
long	adec(long*);
int	cas32(unsigned long*, unsigned long, unsigned long);
int	casp(void**, void*, void*);
int	casl(unsigned long*, unsigned long, unsigned long);
