/*
 * functions (possibly) linked in, complete, from libc.
 */

/*
 * mem routines
 */
extern	void	*memccpy(void*, void*, int, long);
extern	void	*memset(void*, int, long);
extern	int	memcmp(void*, void*, long);
extern	void	*memmove(void*, void*, long);
extern	void	*memchr(void*, int, long);

/*
 * string routines
 */
extern	char	*strcat(char*, char*);
extern	char	*strchr(char*, char);
extern	char	*strrchr(char*, char);
extern	int	strcmp(char*, char*);
extern	char	*strcpy(char*, char*);
extern	char	*strncat(char*, char*, long);
extern	char	*strncpy(char*, char*, long);
extern	int	strncmp(char*, char*, long);
extern	long	strlen(char*);
extern	char*	strstr(char*, char*);
extern	int	atoi(char*);

enum
{
	UTFmax		= 3,	/* maximum bytes per rune */
	Runesync	= 0x80,	/* cannot represent part of a UTF sequence */
	Runeself	= 0x80,	/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80,	/* decoding error in UTF */
};

/*
 * rune routines
 */
extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	char*	utfrune(char*, long);
extern	int	utflen(char*);
extern	int	runelen(long);

extern	int	abs(int);

/*
 * print routines
 */
typedef struct Cconv Fconv;
extern	char*	donprint(char*, char*, char*, void*);
extern	int	sprint(char*, char*, ...);
extern	char*	seprint(char*, char*, char*, ...);
extern	int	snprint(char*, int, char*, ...);
extern	int	print(char*, ...);

/*
 * one-of-a-kind
 */
extern	char*	cleanname(char*);
extern	ulong	getcallerpc(void*);
extern	long	strtol(char*, char**, int);
extern	ulong	strtoul(char*, char**, int);
extern	vlong	strtoll(char*, char**, int);
extern	uvlong	strtoull(char*, char**, int);
extern	char	etext[];
extern	char	edata[];
extern	char	end[];
extern	int	getfields(char*, char**, int, int, char*);

/*
 * Syscall data structures
 */
#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MCACHE	0x0010	/* cache some data */
#define	MMASK	0x001F	/* all bits on */

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */

#define	NCONT	0	/* continue after note */
#define	NDFLT	1	/* terminate after note */
#define	NSAVE	2	/* clear note but hold state */
#define	NRSTR	3	/* restore saved state */

typedef struct Qid	Qid;
typedef struct Dir	Dir;
typedef struct Waitmsg	Waitmsg;

#define	ERRLEN		64
#define	DIRLEN		116
#define	NAMELEN		28

struct Qid
{
	ulong	path;
	ulong	vers;
};

struct Dir
{
	char	name[NAMELEN];
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	Qid	qid;
	ulong	mode;
	long	atime;
	long	mtime;
	vlong	length;
	short	type;
	short	dev;
};

struct Waitmsg
{
	char	pid[12];	/* of loved one */
	char	time[3*12];	/* of loved one and descendants */
	char	msg[ERRLEN];
};

/*
 *  locks
 */
typedef
struct Lock {
	int	val;
} Lock;

extern int	_tas(int*);

extern	void	lock(Lock*);
extern	void	unlock(Lock*);
extern	int	canlock(Lock*);
