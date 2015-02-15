/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"libc.a"
#pragma	src	"/sys/src/libc"

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	offsetof(s, m)	(ulong)(&(((s*)0)->m))
#define	assert(x)	if(x){}else _assert("x")

/*
 * mem routines
 */
extern	void*	memccpy(void*, void*, int, uint32_t);
extern	void*	memset(void*, int, uint32_t);
extern	int	memcmp(void*, void*, uint32_t);
extern	void*	memcpy(void*, void*, uint32_t);
extern	void*	memmove(void*, void*, uint32_t);
extern	void*	memchr(void*, int, uint32_t);

/*
 * string routines
 */
extern	int8_t*	strcat(int8_t*, int8_t*);
extern	int8_t*	strchr(int8_t*, int);
extern	int	strcmp(int8_t*, int8_t*);
extern	int8_t*	strcpy(int8_t*, int8_t*);
extern	int8_t*	strecpy(int8_t*, int8_t*, int8_t*);
extern	int8_t*	strdup(int8_t*);
extern	int8_t*	strncat(int8_t*, int8_t*, int32_t);
extern	int8_t*	strncpy(int8_t*, int8_t*, int32_t);
extern	int	strncmp(int8_t*, int8_t*, int32_t);
extern	int8_t*	strpbrk(int8_t*, int8_t*);
extern	int8_t*	strrchr(int8_t*, int);
extern	int8_t*	strtok(int8_t*, int8_t*);
extern	int32_t	strlen(int8_t*);
extern	int32_t	strspn(int8_t*, int8_t*);
extern	int32_t	strcspn(int8_t*, int8_t*);
extern	int8_t*	strstr(int8_t*, int8_t*);
extern	int	cistrncmp(int8_t*, int8_t*, int);
extern	int	cistrcmp(int8_t*, int8_t*);
extern	int8_t*	cistrstr(int8_t*, int8_t*);
extern	int	tokenize(int8_t*, int8_t**, int);

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
 * rune routines
 */
extern	int	runetochar(int8_t*, Rune*);
extern	int	chartorune(Rune*, int8_t*);
extern	int	runelen(int32_t);
extern	int	runenlen(Rune*, int);
extern	int	fullrune(int8_t*, int);
extern	int	utflen(int8_t*);
extern	int	utfnlen(int8_t*, int32_t);
extern	int8_t*	utfrune(int8_t*, int32_t);
extern	int8_t*	utfrrune(int8_t*, int32_t);
extern	int8_t*	utfutf(int8_t*, int8_t*);
extern	int8_t*	utfecpy(int8_t*, int8_t*, int8_t*);

extern	Rune*	runestrcat(Rune*, Rune*);
extern	Rune*	runestrchr(Rune*, Rune);
extern	int	runestrcmp(Rune*, Rune*);
extern	Rune*	runestrcpy(Rune*, Rune*);
extern	Rune*	runestrncpy(Rune*, Rune*, int32_t);
extern	Rune*	runestrecpy(Rune*, Rune*, Rune*);
extern	Rune*	runestrdup(Rune*);
extern	Rune*	runestrncat(Rune*, Rune*, int32_t);
extern	int	runestrncmp(Rune*, Rune*, int32_t);
extern	Rune*	runestrrchr(Rune*, Rune);
extern	int32_t	runestrlen(Rune*);
extern	Rune*	runestrstr(Rune*, Rune*);

extern	Rune	tolowerrune(Rune);
extern	Rune	totitlerune(Rune);
extern	Rune	toupperrune(Rune);
extern	Rune	tobaserune(Rune);
extern	int	isalpharune(Rune);
extern	int	isbaserune(Rune);
extern	int	isdigitrune(Rune);
extern	int	islowerrune(Rune);
extern	int	isspacerune(Rune);
extern	int	istitlerune(Rune);
extern	int	isupperrune(Rune);

/*
 * malloc
 */
extern	void*	malloc(uint32_t);
extern	void*	mallocz(uint32_t, int);
extern	void	free(void*);
extern	uint32_t	msize(void*);
extern	void*	mallocalign(uint32_t, uint32_t, int32_t, uint32_t);
extern	void*	calloc(uint32_t, uint32_t);
extern	void*	realloc(void*, uint32_t);
extern	void	setmalloctag(void*, uint32_t);
extern	void	setrealloctag(void*, uint32_t);
extern	uint32_t	getmalloctag(void*);
extern	uint32_t	getrealloctag(void*);
extern	void*	malloctopoolblock(void*);

/*
 * print routines
 */
typedef struct Fmt	Fmt;
struct Fmt{
	uint8_t	runes;			/* output buffer is runes or chars? */
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
	uint32_t	flags;
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
	FmtByte		= FmtComma << 1,

	FmtFlag		= FmtByte << 1
};

extern	int	print(int8_t*, ...);
extern	int8_t*	seprint(int8_t*, int8_t*, int8_t*, ...);
extern	int8_t*	vseprint(int8_t*, int8_t*, int8_t*, va_list);
extern	int	snprint(int8_t*, int, int8_t*, ...);
extern	int	vsnprint(int8_t*, int, int8_t*, va_list);
extern	int8_t*	smprint(int8_t*, ...);
extern	int8_t*	vsmprint(int8_t*, va_list);
extern	int	sprint(int8_t*, int8_t*, ...);
extern	int	fprint(int, int8_t*, ...);
extern	int	vfprint(int, int8_t*, va_list);

extern	int	runesprint(Rune*, int8_t*, ...);
extern	int	runesnprint(Rune*, int, int8_t*, ...);
extern	int	runevsnprint(Rune*, int, int8_t*, va_list);
extern	Rune*	runeseprint(Rune*, Rune*, int8_t*, ...);
extern	Rune*	runevseprint(Rune*, Rune*, int8_t*, va_list);
extern	Rune*	runesmprint(int8_t*, ...);
extern	Rune*	runevsmprint(int8_t*, va_list);

extern	int	fmtfdinit(Fmt*, int, int8_t*, int);
extern	int	fmtfdflush(Fmt*);
extern	int	fmtstrinit(Fmt*);
extern	int8_t*	fmtstrflush(Fmt*);
extern	int	runefmtstrinit(Fmt*);
extern	Rune*	runefmtstrflush(Fmt*);

#pragma	varargck	argpos	fmtprint	2
#pragma	varargck	argpos	fprint		2
#pragma	varargck	argpos	print		1
#pragma	varargck	argpos	runeseprint	3
#pragma	varargck	argpos	runesmprint	1
#pragma	varargck	argpos	runesnprint	3
#pragma	varargck	argpos	runesprint	2
#pragma	varargck	argpos	seprint		3
#pragma	varargck	argpos	smprint		1
#pragma	varargck	argpos	snprint		3
#pragma	varargck	argpos	sprint		2

#pragma	varargck	type	"lld"	vlong
#pragma	varargck	type	"llo"	vlong
#pragma	varargck	type	"llx"	vlong
#pragma	varargck	type	"llb"	vlong
#pragma	varargck	type	"lld"	uvlong
#pragma	varargck	type	"llo"	uvlong
#pragma	varargck	type	"llx"	uvlong
#pragma	varargck	type	"llb"	uvlong
#pragma	varargck	type	"ld"	long
#pragma	varargck	type	"lo"	long
#pragma	varargck	type	"lx"	long
#pragma	varargck	type	"lb"	long
#pragma	varargck	type	"ld"	ulong
#pragma	varargck	type	"lo"	ulong
#pragma	varargck	type	"lx"	ulong
#pragma	varargck	type	"lb"	ulong
#pragma	varargck	type	"d"	int
#pragma	varargck	type	"o"	int
#pragma	varargck	type	"x"	int
#pragma	varargck	type	"c"	int
#pragma	varargck	type	"C"	int
#pragma	varargck	type	"b"	int
#pragma	varargck	type	"d"	uint
#pragma	varargck	type	"x"	uint
#pragma	varargck	type	"c"	uint
#pragma	varargck	type	"C"	uint
#pragma	varargck	type	"b"	uint
#pragma	varargck	type	"f"	double
#pragma	varargck	type	"e"	double
#pragma	varargck	type	"g"	double
#pragma	varargck	type	"s"	char*
#pragma	varargck	type	"q"	char*
#pragma	varargck	type	"S"	Rune*
#pragma	varargck	type	"Q"	Rune*
#pragma	varargck	type	"r"	void
#pragma	varargck	type	"%"	void
#pragma	varargck	type	"n"	int*
#pragma	varargck	type	"p"	uintptr
#pragma	varargck	type	"p"	void*
#pragma	varargck	flag	','
#pragma	varargck	flag	' '
#pragma	varargck	flag	'h'
#pragma varargck	type	"<"	void*
#pragma varargck	type	"["	void*
#pragma varargck	type	"H"	void*
#pragma varargck	type	"lH"	void*

extern	int	fmtinstall(int, int (*)(Fmt*));
extern	int	dofmt(Fmt*, int8_t*);
extern	int	dorfmt(Fmt*, Rune*);
extern	int	fmtprint(Fmt*, int8_t*, ...);
extern	int	fmtvprint(Fmt*, int8_t*, va_list);
extern	int	fmtrune(Fmt*, int);
extern	int	fmtstrcpy(Fmt*, int8_t*);
extern	int	fmtrunestrcpy(Fmt*, Rune*);
/*
 * error string for %r
 * supplied on per os basis, not part of fmt library
 */
extern	int	errfmt(Fmt *f);

/*
 * quoted strings
 */
extern	int8_t	*unquotestrdup(int8_t*);
extern	Rune	*unquoterunestrdup(Rune*);
extern	int8_t	*quotestrdup(int8_t*);
extern	Rune	*quoterunestrdup(Rune*);
extern	int	quotestrfmt(Fmt*);
extern	int	quoterunestrfmt(Fmt*);
extern	void	quotefmtinstall(void);
extern	int	(*doquote)(int);
extern	int	needsrcquote(int);

/*
 * random number
 */
extern	void	srand(int32_t);
extern	int	rand(void);
extern	int	nrand(int);
extern	int32_t	lrand(void);
extern	int32_t	lnrand(int32_t);
extern	double	frand(void);
extern	uint32_t	truerand(void);			/* uses /dev/random */
extern	uint32_t	ntruerand(uint32_t);		/* uses /dev/random */

/*
 * math
 */
extern	uint32_t	getfcr(void);
extern	void	setfsr(uint32_t);
extern	uint32_t	getfsr(void);
extern	void	setfcr(uint32_t);
extern	double	NaN(void);
extern	double	Inf(int);
extern	int	isNaN(double);
extern	int	isInf(double, int);
extern	uint32_t	umuldiv(uint32_t, uint32_t, uint32_t);
extern	int32_t	muldiv(int32_t, int32_t, int32_t);

extern	double	pow(double, double);
extern	double	atan2(double, double);
extern	double	fabs(double);
extern	double	atan(double);
extern	double	log(double);
extern	double	log10(double);
extern	double	exp(double);
extern	double	floor(double);
extern	double	ceil(double);
extern	double	hypot(double, double);
extern	double	sin(double);
extern	double	cos(double);
extern	double	tan(double);
extern	double	asin(double);
extern	double	acos(double);
extern	double	sinh(double);
extern	double	cosh(double);
extern	double	tanh(double);
extern	double	sqrt(double);
extern	double	fmod(double, double);

#define	HUGE	3.4028234e38
#define	PIO2	1.570796326794896619231e0
#define	PI	(PIO2+PIO2)

/*
 * Time-of-day
 */

typedef
struct Tm
{
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
	int	wday;
	int	yday;
	int8_t	zone[4];
	int	tzoff;
} Tm;

extern	Tm*	gmtime(int32_t);
extern	Tm*	localtime(int32_t);
extern	int8_t*	asctime(Tm*);
extern	int8_t*	ctime(int32_t);
extern	double	cputime(void);
extern	int32_t	times(int32_t*);
extern	int32_t	tm2sec(Tm*);
extern	int64_t	nsec(void);

extern	void	cycles(uint64_t*);	/* 64-bit value of the cycle counter if there is one, 0 if there isn't */

/*
 * one-of-a-kind
 */
enum
{
	PNPROC		= 1,
	PNGROUP		= 2,
};

extern	void	_assert(int8_t*);
extern	int	abs(int);
extern	int	atexit(void(*)(void));
extern	void	atexitdont(void(*)(void));
extern	int	atnotify(int(*)(void*, int8_t*), int);
extern	double	atof(int8_t*);
extern	int	atoi(int8_t*);
extern	int32_t	atol(int8_t*);
extern	int64_t	atoll(int8_t*);
extern	double	charstod(int(*)(void*), void*);
extern	int8_t*	cleanname(int8_t*);
extern	int	decrypt(void*, void*, int);
extern	int	encrypt(void*, void*, int);
extern	int	dec64(uint8_t*, int, int8_t*, int);
extern	int	enc64(int8_t*, int, uint8_t*, int);
extern	int	dec32(uint8_t*, int, int8_t*, int);
extern	int	enc32(int8_t*, int, uint8_t*, int);
extern	int	dec16(uint8_t*, int, int8_t*, int);
extern	int	enc16(int8_t*, int, uint8_t*, int);
extern	int	encodefmt(Fmt*);
extern	void	exits(int8_t*);
extern	double	frexp(double, int*);
extern	uintptr	getcallerpc(void*);
extern	int8_t*	getenv(int8_t*);
extern	int	getfields(int8_t*, int8_t**, int, int, int8_t*);
extern	int	gettokens(int8_t *, int8_t **, int, int8_t *);
extern	int8_t*	getuser(void);
extern	int8_t*	getwd(int8_t*, int);
extern	int	iounit(int);
extern	int32_t	labs(int32_t);
extern	double	ldexp(double, int);
extern	void	longjmp(jmp_buf, int);
extern	int8_t*	mktemp(int8_t*);
extern	double	modf(double, double*);
extern	int	netcrypt(void*, void*);
extern	void	notejmp(void*, jmp_buf, int);
extern	void	perror(const int8_t*);
extern	int	postnote(int, int, int8_t *);
extern	double	pow10(int);
extern	int	putenv(int8_t*, int8_t*);
extern	void	qsort(void*, int32_t, int32_t,
				int (*)(void*, void*));
extern	int	setjmp(jmp_buf);
extern	double	strtod(int8_t*, int8_t**);
extern	int32_t	strtol(int8_t*, int8_t**, int);
extern	uint32_t	strtoul(int8_t*, int8_t**, int);
extern	int64_t	strtoll(int8_t*, int8_t**, int);
extern	uint64_t	strtoull(int8_t*, int8_t**, int);
extern	void	sysfatal(int8_t*, ...);
#pragma	varargck	argpos	sysfatal	1
extern	void	syslog(int, int8_t*, int8_t*, ...);
#pragma	varargck	argpos	syslog	3
extern	int32_t	time(int32_t*);
extern	int	tolower(int);
extern	int	toupper(int);

/*
 *  profiling
 */
enum {
	Profoff,		/* No profiling */
	Profuser,		/* Measure user time only (default) */
	Profkernel,		/* Measure user + kernel time */
	Proftime,		/* Measure total time */
	Profsample,		/* Use clock interrupt to sample (default when there is no cycle counter) */
}; /* what */
extern	void	prof(void (*fn)(void*), void *arg, int entries, int what);

/*
 * atomic
 */
int32_t	ainc(int32_t*);
int32_t	adec(int32_t*);
int	cas32(uint32_t*, uint32_t, uint32_t);
int	casp(void**, void*, void*);
int	casl(uint32_t*, uint32_t, uint32_t);

/*
 *  synchronization
 */
typedef
struct Lock {
	int32_t	key;
	int32_t	sem;
} Lock;

extern int	_tas(int*);

extern	void	lock(Lock*);
extern	void	unlock(Lock*);
extern	int	canlock(Lock*);

typedef struct QLp QLp;
struct QLp
{
	int	inuse;
	QLp	*next;
	int8_t	state;
};

typedef
struct QLock
{
	Lock	lock;
	int	locked;
	QLp	*head;
	QLp 	*tail;
} QLock;

extern	void	qlock(QLock*);
extern	void	qunlock(QLock*);
extern	int	canqlock(QLock*);
extern	void	_qlockinit(void* (*)(void*, void*));	/* called only by the thread library */

typedef
struct RWLock
{
	Lock	lock;
	int	_readers;	/* number of readers */
	int	writer;		/* number of writers */
	QLp	*head;		/* list of waiting processes */
	QLp	*tail;
} RWLock;

extern	void	rlock(RWLock*);
extern	void	runlock(RWLock*);
extern	int	canrlock(RWLock*);
extern	void	wlock(RWLock*);
extern	void	wunlock(RWLock*);
extern	int	canwlock(RWLock*);

typedef
struct Rendez
{
	QLock	*l;
	QLp	*head;
	QLp	*tail;
} Rendez;

extern	void	rsleep(Rendez*);	/* unlocks r->l, sleeps, locks r->l again */
extern	int	rwakeup(Rendez*);
extern	int	rwakeupall(Rendez*);
extern	void**	privalloc(void);
extern	void	privfree(void**);

/*
 *  network dialing
 */
#define NETPATHLEN 40
extern	int	accept(int, int8_t*);
extern	int	announce(int8_t*, int8_t*);
extern	int	dial(int8_t*, int8_t*, int8_t*, int*);
extern	void	setnetmtpt(int8_t*, int, int8_t*);
extern	int	hangup(int);
extern	int	listen(int8_t*, int8_t*);
extern	int8_t*	netmkaddr(int8_t*, int8_t*, int8_t*);
extern	int	reject(int, int8_t*, int8_t*);

/*
 *  encryption
 */
extern	int	pushssl(int, int8_t*, int8_t*, int8_t*, int*);
extern	int	pushtls(int, int8_t*, int8_t*, int, int8_t*,
				 int8_t*);

/*
 *  network services
 */
typedef struct NetConnInfo NetConnInfo;
struct NetConnInfo
{
	int8_t	*dir;		/* connection directory */
	int8_t	*root;		/* network root */
	int8_t	*spec;		/* binding spec */
	int8_t	*lsys;		/* local system */
	int8_t	*lserv;		/* local service */
	int8_t	*rsys;		/* remote system */
	int8_t	*rserv;		/* remote service */
	int8_t	*laddr;		/* local address */
	int8_t	*raddr;		/* remote address */
};
extern	NetConnInfo*	getnetconninfo(int8_t*, int);
extern	void		freenetconninfo(NetConnInfo*);

/*
 * system calls
 *
 */
#define	STATMAX	65535U	/* max length of machine-independent stat structure */
#define	DIRMAX	(sizeof(Dir)+STATMAX)	/* max length of Dir structure */
#define	ERRMAX	128	/* max length of error string */

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
#define	OEXCL	0x1000	/* or'ed in, exclusive use (create only) */
// #define	OBEHIND	0x2000	/* use write behind for writes [for 9n] */

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
#define QTAUTH		0x08		/* type bit for authentication file */
#define QTTMP		0x04		/* type bit for not-backed-up file */
#define QTFILE		0x00		/* plain file */

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMAUTH		0x08000000	/* mode bit for authentication file */
#define DMTMP		0x04000000	/* mode bit for non-backed-up files */
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

typedef
struct Qid
{
	uint64_t	path;
	uint32_t	vers;
	uint8_t	type;
} Qid;

typedef
struct Dir {
	/* system-modified data */
	uint16_t	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	uint32_t	mode;	/* permissions */
	uint32_t	atime;	/* last read time */
	uint32_t	mtime;	/* last write time */
	int64_t	length;	/* file length */
	int8_t	*name;	/* last element of path */
	int8_t	*uid;	/* owner name */
	int8_t	*gid;	/* group name */
	int8_t	*muid;	/* last modifier name */
} Dir;

/* keep /sys/src/ape/lib/ap/plan9/sys9.h in sync with this -rsc */
typedef
struct Waitmsg
{
	int	pid;		/* of loved one */
	uint32_t	time[3];	/* of loved one & descendants */
	int8_t	*msg;
} Waitmsg;

typedef
struct IOchunk
{
	void	*addr;
	uint32_t	len;
} IOchunk;

extern	void	_exits(int8_t*);

extern	void	abort(void);
extern	int	access(int8_t*, int);
extern	int32_t	alarm(uint32_t);
extern	int	await(int8_t*, int);
extern	int	bind(int8_t*, int8_t*, int);
extern	int	brk(void*);
extern	int	chdir(int8_t*);
extern	int	close(int);
extern	int	create(int8_t*, int, uint32_t);
extern	int	dup(int, int);
extern	int	errstr(int8_t*, uint);
extern	int	exec(int8_t*, int8_t*[]);
extern	int	execl(int8_t*, ...);
extern	int	fork(void);
extern	int	rfork(int);
extern	int	fauth(int, int8_t*);
extern	int	fstat(int, uint8_t*, int);
extern	int	fwstat(int, uint8_t*, int);
extern	int	fversion(int, int, int8_t*, int);
extern	int	mount(int, int, int8_t*, int, int8_t*);
extern	int	unmount(int8_t*, int8_t*);
extern	int	noted(int);
extern	int	notify(void(*)(void*, int8_t*));
extern	int	open(int8_t*, int);
extern	int	fd2path(int, int8_t*, int);
// extern	int	fdflush(int);
extern	int	pipe(int*);
extern	int32_t	pread(int, void*, int32_t, int64_t);
extern	int32_t	preadv(int, IOchunk*, int, int64_t);
extern	int32_t	pwrite(int, void*, int32_t, int64_t);
extern	int32_t	pwritev(int, IOchunk*, int, int64_t);
extern	int32_t	read(int, void*, int32_t);
extern	int32_t	readn(int, void*, int32_t);
extern	int32_t	readv(int, IOchunk*, int);
extern	int	remove(int8_t*);
extern	void*	sbrk(uint32_t);
extern	int32_t	oseek(int, int32_t, int);
extern	int64_t	seek(int, int64_t, int);
extern	void*	segattach(int, int8_t*, void*, uint32_t);
extern	void*	segbrk(void*, void*);
extern	int	segdetach(void*);
extern	int	segflush(void*, uint32_t);
extern	int	segfree(void*, uint32_t);
extern	int	semacquire(int32_t*, int);
extern	int32_t	semrelease(int32_t*, int32_t);
extern	int	sleep(int32_t);
extern	int	stat(int8_t*, uint8_t*, int);
extern	int	tsemacquire(int32_t*, uint32_t);
extern	Waitmsg*	wait(void);
extern	int	waitpid(void);
extern	int32_t	write(int, void*, int32_t);
extern	int32_t	writev(int, IOchunk*, int);
extern	int	wstat(int8_t*, uint8_t*, int);
extern	void*	rendezvous(void*, void*);

extern	Dir*	dirstat(int8_t*);
extern	Dir*	dirfstat(int);
extern	int	dirwstat(int8_t*, Dir*);
extern	int	dirfwstat(int, Dir*);
extern	int32_t	dirread(int, Dir**);
extern	void	nulldir(Dir*);
extern	int32_t	dirreadall(int, Dir**);
extern	int	getpid(void);
extern	int	getppid(void);
extern	void	rerrstr(int8_t*, uint);
extern	int8_t*	sysname(void);
extern	void	werrstr(int8_t*, ...);
#pragma	varargck	argpos	werrstr	1

/* compiler directives on plan 9 */
#define SET(x)  ((x)=0)
#define USED(x) if(x){}else{}
#ifdef __GNUC__
#       if __GNUC__ >= 3
#               undef USED
#               define USED(x) ((void)(x))
#       endif
#endif

extern int8_t *argv0;
/* #define	ARGBEGIN	for((argv0||(argv0=*argv)),argv++,argc--;\ */
#define ARGBEGIN        for((argv0?0:(argv0=*argv)),argv++,argc--;\
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
/* #define	ARGEND		SET(_argt);USED(_argt,_argc,_args);}USED(argv, argc); */
#define ARGEND          SET(_argt);USED(_argt);USED(_argc);USED(_args);}USED(argv);USED(argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	EARGF(x)	(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (int8_t*)0)))

#define	ARGC()		_argc

/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
extern	int8_t	_end[];
/* In order to not to break GNU ld internal linker script */
#define end _end
