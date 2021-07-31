/* avoid name conflicts */
#define accept	pm_accept
#define listen  pm_listen
#define sleep	ksleep
#define wakeup	kwakeup
#define strtod		fmtstrtod

/* conflicts on some os's */
#define encrypt	libencrypt
#define decrypt libdecrypt
#define oserror	liboserror
#define clone	libclone
#define atexit	libatexit
#define log2	liblog2
#define log	liblog
#define reboot	libreboot
#define strtoll libstrtoll
#undef timeradd
#define timeradd	xtimeradd


#define	nil	((void*)0)

typedef unsigned char	p9_uchar;
typedef unsigned int	p9_uint;
typedef unsigned int	p9_ulong;
typedef int		p9_long;
typedef signed char	p9_schar;
typedef unsigned short	p9_ushort;
typedef unsigned int	Rune;
typedef unsigned int	p9_u32int;
typedef p9_u32int mpdigit;

/* make sure we don't conflict with predefined types */
#define schar	p9_schar
#define uchar	p9_uchar
#define ushort	p9_ushort
#define uint	p9_uint
#define u32int	p9_u32int

/* #define long int rather than p9_long so that "unsigned long" is valid */
#define long	int
#define ulong	p9_ulong
#define vlong	p9_vlong
#define uvlong	p9_uvlong

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define SET(x)		((x)=0)
#define	USED(x)		if(x);else

enum
{
	UTFmax		= 4,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0xFFFD,	/* decoding error in UTF */
	Runemax		= 0x10FFFF,	/* 21-bit rune */
	Runemask	= 0x1FFFFF,	/* bits used by runes (see grep) */
};

/*
 * new rune routines
 */
extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	int	runelen(long);
extern	int	fullrune(char*, int);

extern  int	wstrtoutf(char*, Rune*, int);
extern  int	wstrutflen(Rune*);

/*
 * rune routines from converted str routines
 */
extern	long	utflen(char*);
extern	char*	utfrune(char*, long);
extern	char*	utfrrune(char*, long);

/*
 * Syscall data structures
 */
#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MCACHE	0x0010	/* cache some data */
#define	MMASK	0x0017	/* all bits on */

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */
#define	OEXCL   0x1000	/* or'ed in, exclusive create */

#define	NCONT	0	/* continue after note */
#define	NDFLT	1	/* terminate after note */
#define	NSAVE	2	/* clear note but hold state */
#define	NRSTR	3	/* restore saved state */

#define	ERRMAX			128	/* max length of error string */
#define	KNAMELEN		28	/* max length of name held in kernel */

/* bits in Qid.type */
#define QTDIR		0x80		/* type bit for directories */
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTAUTH		0x08		/* type bit for authentication file */
#define QTFILE		0x00		/* plain file */

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMAPPEND		0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMAUTH		0x08000000	/* mode bit for authentication files */
#define DMREAD		0x4		/* mode bit for read permission */
#define DMWRITE		0x2		/* mode bit for write permission */
#define DMEXEC		0x1		/* mode bit for execute permission */

typedef struct Lock
{
#ifdef PTHREAD
	int init;
	pthread_mutex_t mutex;
#else
	long	key;
#endif
} Lock;

typedef struct QLock
{
	Lock	lk;
	struct Proc	*hold;
	struct Proc	*first;
	struct Proc	*last;
} QLock;

typedef
struct Qid
{
	uvlong	path;
	ulong	vers;
	uchar	type;
} Qid;

typedef
struct Dir {
	/* system-modified data */
	ushort	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	ulong	mode;	/* permissions */
	ulong	atime;	/* last read time */
	ulong	mtime;	/* last write time */
	vlong	length;	/* file length */
	char	*name;	/* last element of path */
	char	*uid;	/* owner name */
	char	*gid;	/* group name */
	char	*muid;	/* last modifier name */
} Dir;

typedef
struct Waitmsg
{
	int pid;	/* of loved one */
	ulong time[3];	/* of loved one & descendants */
	char	*msg;
} Waitmsg;

/*
 * print routines
 */
typedef struct Fmt	Fmt;
struct Fmt{
	uchar	runes;			/* output buffer is runes or chars? */
	void	*start;			/* of buffer */
	void	*to;			/* current place in the buffer */
	void	*stop;			/* end of the buffer; overwritten if flush fails */
	int	(*flush)(Fmt *);	/* called when to == stop */
	void	*farg;			/* to make flush a closure */
	int	nfmt;			/* num chars formatted so far */
	va_list	args;			/* args passed to dofmt */
	int	r;			/* % format Rune */
	int	width;
	int	prec;
	ulong	flags;
};

enum{
	FmtWidth	= 1,
	FmtLeft		= FmtWidth << 1,
	FmtPrec		= FmtLeft << 1,
	FmtSharp	= FmtPrec << 1,
	FmtSpace	= FmtSharp << 1,
	FmtSign		= FmtSpace << 1,
	FmtZero		= FmtSign << 1,
	FmtUnsigned	= FmtZero << 1,
	FmtShort	= FmtUnsigned << 1,
	FmtLong		= FmtShort << 1,
	FmtVLong	= FmtLong << 1,
	FmtComma	= FmtVLong << 1,
	FmtByte	= FmtComma << 1,

	FmtFlag		= FmtByte << 1,
	FmtLDouble	= FmtFlag << 1
};

extern	int	print(char*, ...);
extern	char*	seprint(char*, char*, char*, ...);
extern	char*	vseprint(char*, char*, char*, va_list);
extern	int	snprint(char*, int, char*, ...);
extern	int	vsnprint(char*, int, char*, va_list);
extern	char*	smprint(char*, ...);
extern	char*	vsmprint(char*, va_list);
extern	int	sprint(char*, char*, ...);
extern	int	fprint(int, char*, ...);
extern	int	vfprint(int, char*, va_list);

extern	int	(*doquote)(int);
extern	int	runesprint(Rune*, char*, ...);
extern	int	runesnprint(Rune*, int, char*, ...);
extern	int	runevsnprint(Rune*, int, char*, va_list);
extern	Rune*	runeseprint(Rune*, Rune*, char*, ...);
extern	Rune*	runevseprint(Rune*, Rune*, char*, va_list);
extern	Rune*	runesmprint(char*, ...);
extern	Rune*	runevsmprint(char*, va_list);

extern       Rune*	runestrchr(Rune*, Rune);
extern       long	runestrlen(Rune*);
extern       Rune*	runestrstr(Rune*, Rune*);

extern	int	fmtfdinit(Fmt*, int, char*, int);
extern	int	fmtfdflush(Fmt*);
extern	int	fmtstrinit(Fmt*);
extern	int	fmtinstall(int, int (*)(Fmt*));
extern	char*	fmtstrflush(Fmt*);
extern	int	runefmtstrinit(Fmt*);
extern	Rune*	runefmtstrflush(Fmt*);
extern	int	encodefmt(Fmt*);
extern	int	fmtstrcpy(Fmt*, char*);
extern	int	fmtprint(Fmt*, char*, ...);
extern	int	fmtvprint(Fmt*, char*, va_list);
extern	void*	mallocz(ulong, int);

extern	uintptr	getcallerpc(void*);
extern	char*	cleanname(char*);
extern	void	sysfatal(char*, ...);
extern	char*	strecpy(char*, char*, char*);

extern	int	tokenize(char*, char**, int);
extern	int	getfields(char*, char**, int, int, char*);
extern	char*	utfecpy(char*, char*, char*);
extern	long	tas(long*);
extern	void	quotefmtinstall(void);
extern	int	dec64(uchar*, int, char*, int);
extern	int	enc64(char*, int, uchar*, int);
extern	int	dec32(uchar*, int, char*, int);
extern	int	enc32(char*, int, uchar*, int);
extern	int	enc16(char*, int, uchar*, int);
void		hnputs(void *p, unsigned short v);
extern	int	dofmt(Fmt*, char*);
extern	double	__NaN(void);
extern	int	__isNaN(double);
extern	double	strtod(const char*, char**);
extern	int	utfnlen(char*, long);
extern	double	__Inf(int);
extern	int	__isInf(double, int);

extern int (*fmtdoquote)(int);

