#pragma	lib	"libc.a"
#pragma src	"/sys/src/libc"

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

/*
 * mem routines
 */
extern	void*	memccpy(void*, void*, int, ulong);
extern	void*	memset(void*, int, ulong);
extern	int	memcmp(void*, void*, ulong);
extern	void*	memcpy(void*, void*, ulong);
extern	void*	memmove(void*, void*, ulong);
extern	void*	memchr(void*, int, ulong);

/*
 * string routines
 */
extern	char*	strcat(char*, char*);
extern	char*	strchr(char*, char);
extern	int	strcmp(char*, char*);
extern	char*	strcpy(char*, char*);
extern	char*	strdup(char*);
extern	char*	strncat(char*, char*, long);
extern	char*	strncpy(char*, char*, long);
extern	int	strncmp(char*, char*, long);
extern	char*	strpbrk(char*, char*);
extern	char*	strrchr(char*, char);
extern	char*	strtok(char*, char*);
extern	long	strlen(char*);
extern	long	strspn(char*, char*);
extern	long	strcspn(char*, char*);
extern	char*	strstr(char*, char*);
extern	int	tokenize(char*, char**, int);

enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80,		/* decoding error in UTF */
};

/*
 * new rune routines
 */
extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	int	runelen(long);
extern	int	fullrune(char*, int);

/*
 * rune routines from converted str routines
 */
extern	int	utflen(char*);
extern	char*	utfrune(char*, long);
extern	char*	utfrrune(char*, long);
extern	char*	utfutf(char*, char*);

/*
 * malloc
 */
extern	void*	malloc(long);
extern	void	free(void*);
extern	void*	calloc(long, long);
extern	void*	realloc(void*, long);

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
extern	char*	doprint(char*, char*, char*, void*);
extern	int	print(char*, ...);
extern	int	snprint(char*, int, char*, ...);
extern	int	sprint(char*, char*, ...);
extern	int	fprint(int, char*, ...);

extern	int	fmtinstall(int, int (*)(void*, Fconv*));
extern	int	numbconv(void*, Fconv*);
extern	void	strconv(char*, Fconv*);
extern	int	fltconv(void*, Fconv*);
/*
 * random number
 */
extern	void	srand(long);
extern	int	rand(void);
extern	int	nrand(int);
extern	long	lrand(void);
extern	long	lnrand(long);
extern	double	frand(void);

/*
 * math
 */
extern	ulong	getfcr(void);
extern	void	setfsr(ulong);
extern	ulong	getfsr(void);
extern	void	setfcr(ulong);
extern	double	NaN(void);
extern	double	Inf(int);
extern	int	isNaN(double);
extern	int	isInf(double, int);

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
} Tm;

extern	Tm*	gmtime(long);
extern	Tm*	localtime(long);
extern	char*	asctime(Tm*);
extern	char*	ctime(long);
extern	double	cputime(void);
extern	long	times(long*);

/*
 * one-of-a-kind
 */
enum
{
	PNPROC		= 1,
	PNGROUP		= 2,
};

extern	int	abs(int);
extern	int	atexit(void(*)(void));
extern	void	atexitdont(void(*)(void));
extern	int	atnotify(int(*)(void*, char*), int);
extern	double	atof(char*);
extern	int	atoi(char*);
extern	long	atol(char*);
extern	double	charstod(int(*)(void*), void*);
extern	int	decrypt(void*, void*, int);
extern	int	encrypt(void*, void*, int);
extern	void	exits(char*);
extern	double	frexp(double, int*);
extern	char*	getenv(char*);
extern	int	getfields(char*, char**, int);
extern	int	getmfields(char*, char**, int);
extern	char*	getuser(void);
extern	char*	getwd(char*, int);
extern	long	labs(long);
extern	double	ldexp(double, int);
extern	void	longjmp(jmp_buf, int);
extern	char*	mktemp(char*);
extern	double	modf(double, double*);
extern	int	netcrypt(void*, void*);
extern	void	notejmp(void*, jmp_buf, int);
extern	void	perror(char*);
extern  int	postnote(int, int, char *);
extern	double	pow10(int);
extern	int	putenv(char*, char*);
extern	void	qsort(void*, long, long, int (*)(void*, void*));
extern	char*	setfields(char*);
extern	int	setjmp(jmp_buf);
extern	double	strtod(char*, char**);
extern	long	strtol(char*, char**, int);
extern	ulong	strtoul(char*, char**, int);
extern	void	syslog(int, char*, char*, ...);
extern	long	time(long*);
extern	int	tolower(int);
extern	int	toupper(int);


/*
 *  network dialing and authentication
 */
#define NETPATHLEN 40
extern	int	accept(int, char*);
extern	int	announce(char*, char*);
extern	int	dial(char*, char*, char*, int*);
extern	int	hangup(int);
extern	int	listen(char*, char*);
extern	char*	netmkaddr(char*, char*, char*);
extern	int	reject(int, char*, char*);

/*
 * system calls
 *
 */
#define	NAMELEN	28	/* length of path element, including '\0' */
#define	DIRLEN	116	/* length of machine-independent Dir structure */
#define	ERRLEN	64	/* length of error string */

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

/* Segattch */
#define	SG_RONLY	0040	/* read only */
#define	SG_CEXEC	0100	/* detach on exec */

#define	NCONT	0	/* continue after note */
#define	NDFLT	1	/* terminate after note */
#define	NSAVE	2	/* clear note but hold state */
#define	NRSTR	3	/* restore saved state */

#define CHDIR		0x80000000	/* mode bit for directories */
#define CHAPPEND	0x40000000	/* mode bit for append only files */
#define CHEXCL		0x20000000	/* mode bit for exclusive use files */
#define CHMOUNT		0x10000000	/* mode bit for mounted channel */
#define CHREAD		0x4		/* mode bit for read permission */
#define CHWRITE		0x2		/* mode bit for write permission */
#define CHEXEC		0x1		/* mode bit for execute permission */

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
	RFCFDG		= (1<<12)
};

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
	Length;
	ushort	type;
	ushort	dev;
} Dir;

typedef
struct Waitmsg
{
	char	pid[12];	/* of loved one */
	char	time[3*12];	/* of loved one & descendants */
	char	msg[ERRLEN];
} Waitmsg;

extern	void	_exits(char*);

extern	void	abort(void);
extern	int	access(char*, int);
extern	long	alarm(ulong);
extern	int	bind(char*, char*, int);
extern	int	brk(void*);
extern	int	chdir(char*);
extern	int	close(int);
extern	int	create(char*, int, ulong);
extern	int	dup(int, int);
extern	int	errstr(char*);
extern	int	exec(char*, char*[]);
extern	int	execl(char*, ...);
extern  int	filsys(int, int, char*);
extern	int	fork(void);
extern	int	rfork(int);
extern	int	fauth(int, char*);
extern	int	fsession(int, char*);
extern	int	fstat(int, char*);
extern	int	fwstat(int, char*);
extern	int	mount(int, char*, int, char*);
extern	int	unmount(char*, char*);
extern	int	noted(int);
extern	int	notify(void(*)(void*, char*));
extern	int	open(char*, int);
extern	int	pipe(int*);
extern	long	read(int, void*, long);
extern	long	readn(int, void*, long);
#define		read9p read
extern	int	remove(char*);
extern	void*	sbrk(ulong);
extern	long	seek(int, long, int);
extern	long	segattach(int, char*, void*, ulong);
extern	int	segbrk(void*, void*);
extern	int	segdetach(void*);
extern	int	segflush(void*, ulong);
extern	int	segfree(void*, ulong);
extern	int	sleep(long);
extern	int	stat(char*, char*);
extern	int	wait(Waitmsg*);
extern	long	write(int, void*, long);
#define		write9p write
extern	int	wstat(char*, char*);
extern	int	rendezvous(ulong, ulong);

extern	int	dirstat(char*, Dir*);
extern	int	dirfstat(int, Dir*);
extern	int	dirwstat(char*, Dir*);
extern	int	dirfwstat(int, Dir*);
extern	long	dirread(int, Dir*, long);
extern	int	getpid(void);
extern	int	getppid(void);
extern	void	werrstr(char*, ...);

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
#define	ARGEND		SET(_argt);USED(_argt,_argc,_args);}USED(argv, argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc
