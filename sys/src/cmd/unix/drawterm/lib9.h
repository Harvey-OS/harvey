#include "os.h"

/* avoid name conflicts */
#define accept	pm_accept
#define listen  pm_listen

/* sys calls */
#define	bind	sysbind
#define	chdir	syschdir
#define	close	sysclose
#define create	syscreate
#define dup	sysdup
#define export	sysexport
#define fstat	sysfstat
#define fwstat	sysfwstat
#define mount	sysmount
#define	open	sysopen
#define start	sysstart
#define read	sysread
#define remove	sysremove
#define seek	sysseek
#define stat	sysstat
#define	write	syswrite
#define wstat	syswstat
#define unmount	sysunmount
#define pipe	syspipe

/* conflicts on some os's */
#define encrypt	libencrypt
#define oserror	liboserror
#define clone	libclone
#define atexit	libatexit
#define log2	liblog2


#define	nil	((void*)0)

typedef unsigned char	p9_uchar;
typedef unsigned int	p9_uint;
typedef unsigned int	p9_ulong;
typedef int		p9_long;
typedef signed char	p9_schar;
typedef unsigned short	p9_ushort;
typedef unsigned short	Rune;

/* make sure we don't conflict with predefined types */
#define schar	p9_schar
#define uchar	p9_uchar
#define ushort	p9_ushort
#define uint	p9_uint

/* #define long int rather than p9_long so that "unsigned long" is valid */
#define long	int
#define ulong	p9_ulong
#define vlong	p9_vlong
#define uvlong	p9_uvlong

long p9sleep(long);

typedef	struct Lock	Lock;
typedef struct Qlock	Qlock;
typedef struct Ref	Ref;
typedef struct Rendez	Rendez;

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	USED(x)		if(x);else
#define	SET(x)

enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80		/* decoding error in UTF */
};

/*
 * new rune routines
 */
extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	int	runelen(Rune);
extern	int	fullrune(char*, int);

extern  int	wstrtoutf(char*, Rune*, int);
extern  int	wstrutflen(Rune*);

/*
 * rune routines from converted str routines
 */
extern	long	utflen(char*);
extern	char*	utfrune(char*, Rune);
extern	char*	utfrrune(char*, Rune);

#define	MAXFDATA (8*1024) /* max length of read/write Blocks */
#define	NAMELEN	28	/* length of path element, including '\0' */
#define	DIRLEN	116	/* length of machine-independent Dir structure */
#define	ERRLEN	64	/* length of error string */

#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define MRECOV	0x0008	/* perform recovery if mount channel is lost */
#define MCACHE	0x0010	/* cache some data */
#define	MMASK	0x0007	/* all bits on */

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */

#define CHDIR		0x80000000	/* mode bit for directories */
#define CHAPPEND	0x40000000	/* mode bit for append only files */
#define CHEXCL		0x20000000	/* mode bit for exclusive use files */
#define CHMOUNT		0x10000000	/* mode bit for mounted channel */
#define CHREAD		0x4		/* mode bit for read permission */
#define CHWRITE		0x2		/* mode bit for write permission */
#define CHEXEC		0x1		/* mode bit for execute permission */

typedef
struct Qid
{
	ulong	path;
	ulong	vers;
} Qid;

typedef
struct Dir
{
	char	name[NAMELEN];
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	Qid	qid;
	ulong	mode;
	int	atime;
	int	mtime;
	vlong length;
	ushort	type;
	ushort	dev;
} Dir;

extern	int	bind(char*, char*, int);
extern	int	chdir(char*);
extern	int	close(int);
extern	int	create(char*, int, ulong);
extern	int	dup(int, int);
extern  int	export(int);
extern	int	fstat(int, char*);
extern	int	fwstat(int, char*);
extern	int	mount(int, char*, int, char*);
extern	int	unmount(char*, char*);
extern	int	open(char*, int);
extern	int	pipe(int*);
extern	long	read(int, void*, long);
extern	long	readn(int, void*, long);
extern	int	remove(char*);
extern	long	seek(int, long, int);
extern	int	stat(char*, char*);
extern	long	write(int, void*, long);
extern	int	wstat(char*, char*);

extern	int	dirstat(char*, Dir*);
extern	int	dirfstat(int, Dir*);
extern	int	dirwstat(char*, Dir*);
extern	int	dirfwstat(int, Dir*);
extern	long	dirread(int, Dir*, long);

/*
 *  network dialing and authentication
 */
#define NETPATHLEN 40
extern	int	accept(int, char*);
extern	int	announce(char*, char*);
extern	int	dial(char*, char*, char*, int*);
extern	int	hangup(int);
extern	int	listen(char*, char*);
extern	void	netmkaddr(char*, char*, char*, char*);
extern	int	reject(int, char*, char*);

/*
 * print routines
 */
typedef	struct	Fconv	Fconv;
struct	Fconv
{
	char*	out;		/* pointer to next output */
	char*	eout;		/* pointer to end */
	int	f1;
	int	f2;
	int	f3;
	int	chr;
};

struct Lock
{
	int	val;
};

struct Ref
{
	Lock	l;
	long	ref;
};

struct Rendez
{
	Lock	l;
	void*	t;
};

struct Qlock
{
	Lock	lk;
	void	*hold;
	void	*first;			/* next thread waiting for object */
	void	*last;			/* last thread waiting for object */
};

extern	char*	doprint(char*, char*, char*, va_list);
extern	int	print(char*, ...);
extern	int	snprint(char*, int, char*, ...);
extern	int	sprint(char*, char*, ...);
extern	int	fprint(int, char*, ...);

extern	int	fmtinstall(int, int (*)(va_list*, Fconv*));
extern	int	numbconv(va_list*, Fconv*);
extern	void	strconv(char*, Fconv*);
extern	int	fltconv(va_list*, Fconv*);

extern	int	getfields(char*, char **, int, int, char*);

extern	double	pow10(int);
extern	int	isNaN(double f);
extern	int	isInf(double f, int sign);

extern	void	*mallocz(size_t);
extern  void    szero(void *p, long n);

extern	double	realtime(void);

extern	long	readn(int, void*, long);


extern void	lock(Lock*);
extern void	unlock(Lock*);
extern int	canlock(Lock*);

extern int	refinc(Ref*);
extern int	refdec(Ref*);

extern void	qlock(Qlock*);
extern void	qunlock(Qlock*);
extern int	canqlock(Qlock*);
extern int	holdqlock(Qlock*);

extern	void	rendsleep(Rendez*, int (*f)(void*), void*);
extern	void	rendwakeup(Rendez*);

extern	int	errstr(char*);
extern	void	werrstr(char*, ...);
extern	void	oserror(void);

extern  void	threadinit(void);
extern  int	thread(char*, void(*f)(void*), void*);
extern  void	threadwait(int);
extern	void	threadexit(void);
extern  char	*threaderr(void);
extern	void	*curthread(void);
extern  void	exits(char*);
extern  void	_exits(char*);
extern	int	atexit(void(*)(void));
extern	void	atexitdont(void(*)(void));
extern  void	fatal(char*, ...);

extern	int	atlocalconsole(void);	/* provided by screen*.c */

/*
 * exception interface
 */
#define	waserror()	(setjmp(pm_waserror()->buf))

typedef struct Jump Jump;
struct Jump {
	jmp_buf buf;
};

extern Jump *pm_waserror(void);
extern void	nexterror(void);

extern void	error(char *fmt, ...);
extern void	poperror(void);


extern	char*	getuser(void);
extern ulong	getcallerpc(void*);
extern	int	decrypt(void*, void*, int);
extern	int	encrypt(void*, void*, int);

extern	char	*authaddr;
extern	char	*cpuaddr;

extern char *argv0;
#define	ARGBEGIN	for((argv0? 0: (argv0=*argv)),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				Rune _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(*_args && (_args += chartorune(&_argc, _args)))\
				switch(_argc)
#define	ARGEND		SET(_argt);USED(_argt); USED(_argc); USED(_args);}USED(argv); USED(argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc
