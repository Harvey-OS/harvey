#pragma lib "/$M/lib/alef/libA.a"

enum
{
	NAMELEN	=	28,
	DIRLEN	=	116,
	ERRLEN	=	64,

	/* bind/mount */
	MORDER	=	0x0003,
	MREPL	=	0x0000,
	MBEFORE	=	0x0001,
	MAFTER	=	0x0002,
	MCREATE	=	0x0004,
	MMASK	=	0x0007,

	/* open */
	OREAD	=	0,
	OWRITE	=	1,
	ORDWR	=	2,
	OEXEC	=	3,
	OTRUNC	=	16,
	OCEXEC	=	32,
	ORCLOSE	=	64,

	/* notes */
	NCONT	=	0,
	NDFLT	=	1,

	/* rfork */
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

	/* File modes */
	CHDIR	= 0x80000000,
	CHAPPEND= 0x40000000,
	CHEXCL	= 0x20000000,
	CHREAD	= 0x4,
	CHWRITE	= 0x2,
	CHEXEC	= 0x1
};

union Length
{
	char	clength[8];
	aggr
	{
		int	hlength;
		int	length;
	};
};

aggr Qid
{
	uint	path;
	uint	vers;
};

aggr Dir
{
	char	name[NAMELEN];
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	Qid	qid;
	uint	mode;
	int	atime;
	int	mtime;
	Length;
	usint	type;
	usint	dev;
};

aggr Waitmsg
{
	char	pid[12];
	char	time[3*12];
	char	msg[ERRLEN];
};

/* system calls */
void	abort(void);
int	access(char*, int);
int	alarm(uint);
int	bind(char*, char*, int);
int	brk(void*);
int	brk_(void*);
int	chdir(char*);
int	close(int);
int	create(char*, int, uint);
int	dup(int, int);
int	errstr(char*);
int	exec(char*, char**);
int	execl(char*, ...);
void	exits(char*);
void	_exits(char*);
void	__exits(char*);
int	fork(void);
int	rfork(int);
int	fstat(int, char*);
int	fwstat(int, char*);
int	mount(int, char*, int, char*, char*);
int	unmount(char*, char*);
int	noted(int);
int	open(char*, int);
int	pipe(int*);
int	read(int, void*, int);
int	remove(char*);
int	rendezvous(void*, uint);
void*	sbrk(uint);
int	seek(int, uint, int);
int	segattach(int, char*, void*, uint);
int	segbrk(void*, void*);
int	segdetach(void*);
int	segflush(void*, uint);
int	segfree(void*, uint);
int	sleep(int);
int	stat(char*, char*);
int	wait(Waitmsg*);
int	write(int, void*, int);
int	wstat(char*, char*);
int	atexit(void(*)(void));
void	atexitdont(void(*)(void));
int	dirstat(char*, Dir*);
int	dirfstat(int, Dir*);
int	dirwstat(char*, Dir*);
int	dirfwstat(int, Dir*);
int	dirread(int, Dir*, int);
int	convM2D(char*, Dir*);
int	getpid(void);
char	*getuser(void);
int	times(int*);
int	atoi(char*);
int	time(int*);
/*
 * print routines
 */
aggr Printspec
{
	void	*o;
	int	f1, f2, f3;
	int	chr;

	char	*out;
	char	*eout;
};

int	print(char*, ...);
int	fprint(int, char*, ...);
int	sprint(char*, char*, ...);
char*	doprint(char*, char*, char*, void*);
void	strconv(Printspec *p, char*);
int	fmtinstall(int, int (*)(Printspec*));
/*
 * mem routines
 */
void*	memccpy(void*, void*, int, uint);
void*	memset(void*, int, uint);
int	memcmp(void*, void*, uint);
void*	memcpy(void*, void*, uint);
void*	memmove(void*, void*, uint);
void*	memchr(void*, int, uint);
/*
 * string routines
 */
char*	strcat(char*, char*);
char*	strchr(char*, char);
int	strcmp(char*, char*);
char*	strcpy(char*, char*);
char*	strdup(char*);
char*	strncat(char*, char*, int);
char*	strncpy(char*, char*, int);
int	strncmp(char*, char*, int);
char*	strpbrk(char*, char*);
char*	strrchr(char*, char);
char*	strtok(char*, char*);
int	strlen(char*);
int	strspn(char*, char*);
int	strcspn(char*, char*);
char*	strstr(char*, char*);
int	strtoi(char*, char**, int);
/*
 * math
 */
float	pow10(int);
float	NaN(void);
float	Inf(int);
int	isNaN(float);
int	isInf(float, int);
float	floor(float);
float	frexp(float, int*);
int	abs(int);
float	modf(float, float*);

/*
 * memory allocation
 */
void	*malloc(uint);
void	free(void*);
void	*realloc(void*, uint);
/*
 *  network dialing and authentication
 */
#define NETPATHLEN 40
int	accept(int, char*);
int	announce(char*, char*);
int	dial(char*, char*, char*, int*);
int	hangup(int);
int	listen(char*, char*);
char*	netmkaddr(char*, char*, char*);
int	reject(int, char*, char*);

char*	getenv(char*);
int	putenv(char*, char*);
/*
 * new rune routines
 */
typedef usint Rune;
enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80,		/* decoding error in UTF */
};
int	runetochar(char*, Rune*);
int	chartorune(Rune*, char*);
int	runelen(int);
int	fullrune(char*, int);
/*
 * rune routines from converted str routines
 */
int	utflen(char*);		/* was countrune */
char	*utfrune(char*, int);
char	*utfrrune(char*, int);
char	*utfutf(char*, char*);
/*
 * synchronisation
 */
adt Lock
{
	int	val;

	void	lock(*Lock);
	void	unlock(*Lock);
};

adt QLock
{
	Lock	use;
	int	used;
	void	*queue;

	void	lock(*QLock);
	void	unlock(*QLock);
};

adt Ref
{
	Lock;
	int	ref;

	int	inc(*Ref);
	int	dec(*Ref);
	int	val(*Ref);
};
