/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	offsetof(s, m)	(uintptr_t)(&(((s*)0)->m))
#define	assert(x)	do{if(!(x))_assert(#x);}while(0)

/*
 * mem routines
 */
extern	void*	memccpy(void*, const void*, int, u32);
extern	void*	memset(void*, int, usize);
extern	int	memcmp(const void*, const void*, u32);
extern	void*	memcpy(void*, const void*, usize);
extern	void*	memmove(void*, const void*, usize);
extern	void*	memchr(const void*, int, u32);

/*
 * string routines
 */
extern	char*	strcat(char*, const char*);
extern	char*	strchr(const char*, int);
extern	int	strcmp(const char*, const char*);
extern	char*	strcpy(char*, const char*);
extern	char*	strecpy(char*, char *, const char*);
extern	char*	strdup(const char*);
extern	usize	strlcat(char *, const char *, usize);
extern	usize	strlcpy(char *, const char *, usize);
extern	char*	strncat(char*, const char*, i32);
extern	char*	strncpy(char*, const char*, u32);
extern	int	strncmp(const char*, const char*, i32);
extern	char*	strpbrk(const char*, const char*);
extern	char*	strrchr(const char*, int);
extern	char*	strtok(char*, char*);
extern	int	strlen(const char*);
extern	int	strspn(const char*, const char*);
extern	int	strcspn(const char*, const char*);
extern	char*	strstr(const char*, const char*);
extern	int	cistrncmp(const char*, const char*, int);
extern	int	cistrcmp(const char*, const char*);
extern	char*	cistrstr(const char*, const char*);
extern	int	tokenize(char*, char**, int);

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
extern	int	runetochar(char*, const Rune*);
extern	int	chartorune(Rune*, const char*);
extern	int	runelen(Rune);
extern	int	runenlen(const Rune*, int);
extern	int	fullrune(const char*, int);
extern	int	utflen(const char*);
extern	int	utfnlen(const char*, i32);
extern	char*	utfrune(const char*, Rune);
extern	char*	utfrrune(const char*, Rune);
extern	char*	utfutf(const char*, const char*);
extern	char*	utfecpy(char*, char *, const char*);

extern	Rune*	runestrcat(Rune*, const Rune*);
extern	Rune*	runestrchr(const Rune*, Rune);
extern	int	runestrcmp(const Rune*, const Rune*);
extern	Rune*	runestrcpy(Rune*, const Rune*);
extern	Rune*	runestrncpy(Rune*, const Rune*, i32);
extern	Rune*	runestrecpy(Rune*, Rune*, const Rune*);
extern	Rune*	runestrdup(const Rune*);
extern	Rune*	runestrncat(Rune*, const Rune*, i32);
extern	int	runestrncmp(const Rune*, const Rune*, i32);
extern	Rune*	runestrrchr(const Rune*, Rune);
extern	i32	runestrlen(const Rune*);
extern	Rune*	runestrstr(const Rune*, const Rune*);

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
extern	void*	malloc(usize);
extern	void*	mallocz(u32, int);
extern	void	free(void*);
extern	u32	msize(void*);
extern	void*	mallocalign(u32, u32, i32, u32);
extern	void*	calloc(u32, usize);
extern	void*	realloc(void*, usize);
void	setmalloctag(void*, uintptr_t);
void	setrealloctag(void*, uintptr_t);
uintptr_t	getmalloctag(void*);
uintptr_t	getrealloctag(void*);
void*	malloctopoolblock(void*);

/*
 * print routines
 */
typedef struct Fmt	Fmt;
struct Fmt{
	u8	runes;			/* output buffer is runes or chars? */
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
	u32	flags;
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

extern	int	print(const char*, ...);
extern	char*	seprint(char*, char*, const char*, ...);
extern	char*	vseprint(char*, char*, const char*, va_list);
extern	int	snprint(char*, int, const char*, ...);
extern	int	vsnprint(char*, int, const char*, va_list);
extern	char*	smprint(const char*, ...);
extern	char*	vsmprint(const char*, va_list);
extern	int	sprint(char*, const char*, ...);
extern	int	fprint(int, const char*, ...);
extern	int	vfprint(int, const char*, va_list);

extern	int	runesprint(Rune*, const char*, ...);
extern	int	runesnprint(Rune*, int, const char*, ...);
extern	int	runevsnprint(Rune*, int, const char*, va_list);
extern	Rune*	runeseprint(Rune*, Rune*, const char*, ...);
extern	Rune*	runevseprint(Rune*, Rune*, const char*, va_list);
extern	Rune*	runesmprint(const char*, ...);
extern	Rune*	runevsmprint(const char*, va_list);

extern	int	fmtfdinit(Fmt*, int, char*, int);
extern	int	fmtfdflush(Fmt*);
extern	int	fmtstrinit(Fmt*);
extern	char*	fmtstrflush(Fmt*);
extern	int	runefmtstrinit(Fmt*);
extern	Rune*	runefmtstrflush(Fmt*);



extern	int	fmtinstall(int, int (*)(Fmt*));
extern	int	dofmt(Fmt*, const char*);
extern	int	dorfmt(Fmt*, const Rune*);
extern	int	fmtprint(Fmt*, const char*, ...);
extern	int	fmtvprint(Fmt*, const char*, va_list);
extern	int	fmtrune(Fmt*, int);
extern	int	fmtstrcpy(Fmt*, const char*);
extern	int	fmtrunestrcpy(Fmt*, const Rune*);
/*
 * error string for %r
 * supplied on per os basis, not part of fmt library
 */
extern	int	errfmt(Fmt *f);

/*
 * quoted strings
 */
extern	char	*unquotestrdup(const char*);
extern	Rune	*unquoterunestrdup(const Rune*);
extern	char	*quotestrdup(const char*);
extern	Rune	*quoterunestrdup(const Rune*);
extern	int	quotestrfmt(Fmt*);
extern	int	quoterunestrfmt(Fmt*);
extern	void	quotefmtinstall(void);
extern	int	(*doquote)(int);
extern	int	needsrcquote(int);

/*
 * random number
 */
extern	void	srand(i32);
extern	int	rand(void);
extern	int	nrand(int);
extern	i32	lrand(void);
extern	i32	lnrand(i32);
extern	double	frand(void);
extern	u32	truerand(void);			/* uses /dev/random */
extern	u32	ntruerand(u32);		/* uses /dev/random */

/*
 * math
 */
extern	u32	getfcr(void);
extern	void	setfsr(u32);
extern	u32	getfsr(void);
extern	void	setfcr(u32);
extern	double	NaN(void);
extern	double	Inf(int);
extern	int	isNaN(double);
extern	int	isInf(double, int);
extern	u32	umuldiv(u32, u32, u32);
extern	i32	muldiv(i32, i32, i32);

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
	char	zone[4];
	int	tzoff;
} Tm;

extern	Tm*	gmtime(i32);
extern	Tm*	localtime(i32);
extern	char*	asctime(Tm*);
extern	char*	ctime(i32);
extern	double	cputime(void);
extern	i32	times(i32*);
extern	i32	tm2sec(Tm*);
extern	i64	nsec(void);

extern	void	cycles(u64*);	/* 64-bit value of the cycle counter if there is one, 0 if there isn't */

/*
 * one-of-a-kind
 */
enum
{
	PNPROC		= 1,
	PNGROUP		= 2,
};

extern	void	_assert(char*);
extern	int	abs(int);
extern	int	atexit(void(*)(void));
extern	void	atexitdont(void(*)(void));
extern	int	atnotify(int(*)(void*, char*), int);
extern	double	atof(const char*);
extern	int	atoi(const char*);
extern	i32	atol(const char*);
extern	i64	atoll(const char*);
extern	double	charstod(int(*)(void*), void*);
extern	char*	cleanname(char*);
extern	int	decrypt(void*, void*, int);
extern	int	encrypt(void*, void*, int);
extern	int	dec64(u8*, int, const char*, int);
extern	int	enc64(char*, int, const u8*, int);
extern	int	dec32(u8*, int, const char*, int);
extern	int	enc32(char*, int, const u8*, int);
extern	int	dec16(u8*, int, const char*, int);
extern	int	enc16(char*, int, const u8*, int);
extern	int	encodefmt(Fmt*);
extern	void	exits(const char*);
extern	double	frexp(double, int*);
extern	void	getcallstack(uintptr *, usize);
extern	char*	getenv(const char*);
extern	int	getfields(char*, char**, int, int, const char*);
extern	int	gettokens(char *, char **, int, const char *);
extern	char*	getuser(void);
extern	char*	getwd(char*, int);
extern	int	iounit(int);
extern	i32	labs(i32);
extern	double	ldexp(double, int);
extern	void	longjmp(jmp_buf, int);
extern	char*	mktemp(char*);
extern	double	modf(double, double*);
extern	int	netcrypt(void*, void*);
extern	void	notejmp(void*, jmp_buf, int);
extern	void	perror(const char*);
extern	int	postnote(int, int, const char *);
extern	double	pow10(int);
extern	int	putenv(const char*, const char*);
extern	void	qsort(void*, i32, i32,
				int (*)(const void*, const void*));
extern	int	setjmp(jmp_buf) __attribute__((returns_twice));;
extern	double	strtod(const char*, char**);
extern	i32	strtol(const char*, char**, int);
extern	u32	strtoul(const char*, char**, int);
extern	i64	strtoll(const char*, char**, int);
extern	u64	strtoull(const char*, char**, int);
extern	void	sysfatal(const char*, ...);
extern	void	syslog(int, const char*, const char*, ...);
extern	i32	time(i32*);
extern	int	tolower(int);
extern	int	toupper(int);

/*
 * atomic
 */
i32	ainc(i32*);
i32	adec(i32*);
int	cas32(u32*, u32, u32);
int	casp(void**, void*, void*);
int	casl(u32*, u32, u32);

/*
 *  synchronization
 */
typedef
struct Lock {
	i32	key;
	i32	sem;
} Lock;

extern int	_tas(int*);
extern int	_tas32(int*);

extern	void	lock(Lock*);
extern	void	unlock(Lock*);
extern	int	canlock(Lock*);

typedef struct QLp QLp;
struct QLp
{
	int	inuse;
	QLp	*next;
	char	state;
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
extern	int	accept(int, const char*);
extern	int	announce(const char*, char*);
extern	int	dial(const char*, const char*, char*, int*);
extern	void	setnetmtpt(char*, int, const char*);
extern	int	hangup(int);
extern	int	listen(const char*, char*);
extern	char*	netmkaddr(const char*, const char*, const char*);
extern	int	reject(int, const char*, const char*);

/*
 *  encryption
 */
extern	int	pushssl(int, const char*, const char*, const char*, int*);
extern	int	pushtls(int, const char*, const char*, int, const char*,
				 char*);

/*
 *  network services
 */
typedef struct NetConnInfo NetConnInfo;
struct NetConnInfo
{
	char	*dir;		/* connection directory */
	char	*root;		/* network root */
	char	*spec;		/* binding spec */
	char	*lsys;		/* local system */
	char	*lserv;		/* local service */
	char	*rsys;		/* remote system */
	char	*rserv;		/* remote service */
	char	*laddr;		/* local address */
	char	*raddr;		/* remote address */
};
extern	NetConnInfo*	getnetconninfo(const char*, int);
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
#define QTSYMLINK	0x02		/* type bit for symlink */
#define QTFILE		0x00		/* plain file */

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMAUTH		0x08000000	/* mode bit for authentication file */
#define DMTMP		0x04000000	/* mode bit for non-backed-up files */
#define DMSYMLINK	0x02000000	/* mode bit for symlnk */
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
	u64	path;
	u32	vers;
	u8	type;
} Qid;

typedef
struct Dir {
	/* system-modified data */
	u16	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	u32	mode;	/* permissions */
	u32	atime;	/* last read time */
	u32	mtime;	/* last write time */
	i64	length;	/* file length */
	char	*name;	/* last element of path */
	char	*uid;	/* owner name */
	char	*gid;	/* group name */
	char	*muid;	/* last modifier name */
} Dir;

/* keep /sys/src/ape/lib/ap/plan9/sys9.h in sync with this -rsc */
typedef
struct Waitmsg
{
	int	pid;		/* of loved one */
	u32	time[3];	/* of loved one & descendants */
	char	*msg;
} Waitmsg;

typedef
struct IOchunk
{
	void	*addr;
	u32	len;
} IOchunk;

extern	void	_exits(const char*);

extern	void	abort(void);
extern	int	access(const char*, int);
extern	i64	alarm(u64);
extern	int	await(char*, int);
extern	int	bind(const char*, const char*, int);
extern	int	brk(void*);
extern	int	chdir(const char*);
extern	int	close(int);
extern	int	create(const char*, int, u32);
extern	int	dup(int, int);
extern	char*	errno2str(uint errno);
extern	int	errstr(char*, uint);
extern	int	exec(const char*, char* const[]);
extern	int	execl(const char*, ...);
extern	int	fork(void);
extern	int	rfork(int);
extern	int	fauth(int, const char*);
extern	int	fstat(int, u8*, int);
extern	int	fwstat(int, u8*, int);
extern	int	fversion(int, int, char*, int);
extern	int	mount(int, int, const char*, int, const char*, int);
extern	int	unmount(const char*, const char*);
extern	int	noted(int);
extern	int	notify(void(*)(void*, char*));
extern	int	open(const char*, int);
extern	int	fd2path(int, char*, int);
// extern	int	fdflush(int);
extern	int	pipe(int*);
extern	i32	pread(int, void*, i32, i64);
extern	i32 read(int, void*, i32);
extern	i32	preadv(int, IOchunk*, int, i64);
extern	i32	pwrite(int, const void*, i32, i64);
extern	i32 write(int, const void*, i32);
extern	i32	pwritev(int, IOchunk*, int, i64);
extern	i32	r0(void);
extern	i32	readn(int, void*, i32);
extern	i32	readv(int, IOchunk*, int);
extern	int	remove(const char*);
extern	void*	sbrk(u32);
extern	i32	oseek(int, i32, int);
extern	i64	seek(int, i64, int);
extern	void*	segattach(int, const char*, void*, u32);
extern	void*	segbrk(void*, void*);
extern	int	segdetach(void*);
extern	int	segflush(void*, u32);
extern	int	segfree(void*, u32);
extern	int	semacquire(i32*, int);
extern	i32	semrelease(i32*, i32);
extern	int	sleep(i32);
extern	int	stat(const char*, u8*, int);
extern	int	tsemacquire(i32*, u64);
extern	Waitmsg*	wait(void);
extern	int	waitpid(void);
extern	i32	writev(int, IOchunk*, int);
extern	int	wstat(const char*, u8*, int);
extern	void*	rendezvous(void*, void*);

extern	Dir*	dirstat(const char*);
extern	Dir*	dirfstat(int);
extern	int	dirwstat(const char*, Dir*);
extern	int	dirfwstat(int, Dir*);
extern	i32	dirread(int, Dir**);
extern	void	nulldir(Dir*);
extern	i32	dirreadall(int, Dir**);
extern	int	getpid(void);
extern	int	getppid(void);
extern	void	rerrstr(char*, uint);
extern	char*	sysname(void);
extern	void	werrstr(const char*, ...);

/* compiler directives on plan 9 */
#define SET(x)  ((x)=0)
#define USED(x) ((void)(x))

extern char *argv0;
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
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (char*)0)))

#define	ARGC()		_argc

/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
extern	char	end[];

void* reallocarray(void *base, usize nel, usize size);

typedef struct PSlice PSlice;

struct PSlice {
	void **ptrs;
	usize len;
	usize capacity;
};

void psliceinit(PSlice *slice);
void psliceclear(PSlice *slice);
void *psliceget(PSlice *slice, usize i);
int psliceput(PSlice *slice, usize i, void *p);
int pslicedel(PSlice *slice, usize i);
void psliceappend(PSlice *s, void *p);
usize pslicelen(PSlice *slice);
void **pslicefinalize(PSlice *slice);
void pslicedestroy(PSlice *slice);

/* configstring functions. These are from riscv but the idea is so good
 * it might have other uses. */
void query_mem(const char *config_string, uintptr_t *base, usize *size);
void query_rtc(const char *config_string, uintptr_t *mtime);
int query_uint(const char *configstring, char *name, uintptr_t *m);
