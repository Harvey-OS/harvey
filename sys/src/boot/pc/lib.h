/*
 * functions (possibly) linked in, complete, from libc.
 */

/*
 * mem routines
 */
extern	void*	memccpy(void*, void*, int, long);
extern	void*	memset(void*, int, long);
extern	int	memcmp(void*, void*, long);
extern	void*	memmove(void*, void*, long);
extern	void*	memchr(void*, int, long);

/*
 * string routines
 */
extern	char*	strcat(char*, char*);
extern	char*	strchr(char*, char);
extern	int	strcmp(char*, char*);
extern	char*	strcpy(char*, char*);
extern	char*	strncat(char*, char*, long);
extern	char*	strncpy(char*, char*, long);
extern	int	strncmp(char*, char*, long);
extern	long	strlen(char*);
extern	char*	strrchr(char*, char);
extern	char*	strstr(char*, char*);

/*
 * print routines
 * 	Fconv isn't used but is defined to satisfy prototypes in libg.h
 *	that are never called.
 */
typedef	struct Fconv Fconv;

extern	char*	donprint(char*, char*, char*, void*);
extern	int	sprint(char*, char*, ...);
extern 	int	snprint(char*, int, char*, ...);
extern	int	print(char*, ...);

#define	PRINTSIZE	256
#pragma varargck argpos print 1
#pragma varargck argpos snprint 3
#pragma varargck argpos sprint 2

#pragma	varargck	type	"lld"	vlong
#pragma	varargck	type	"llx"	vlong
#pragma	varargck	type	"lld"	uvlong
#pragma	varargck	type	"llx"	uvlong
#pragma	varargck	type	"ld"	long
#pragma	varargck	type	"lx"	long
#pragma	varargck	type	"ld"	ulong
#pragma	varargck	type	"lx"	ulong
#pragma	varargck	type	"d"	int
#pragma	varargck	type	"x"	int
#pragma	varargck	type	"c"	int
#pragma	varargck	type	"C"	int
#pragma	varargck	type	"d"	uint
#pragma	varargck	type	"x"	uint
#pragma	varargck	type	"c"	uint
#pragma	varargck	type	"C"	uint
#pragma	varargck	type	"f"	double
#pragma	varargck	type	"e"	double
#pragma	varargck	type	"g"	double
#pragma	varargck	type	"s"	char*
#pragma	varargck	type	"q"	char*
#pragma	varargck	type	"S"	Rune*
#pragma	varargck	type	"Q"	Rune*
#pragma	varargck	type	"r"	void
#pragma	varargck	type	"%"	void
#pragma	varargck	type	"|"	int
#pragma	varargck	type	"p"	void*
#pragma varargck	type	"lux"	void*

/*
 * one-of-a-kind
 */
extern	int	atoi(char*);
extern	long	strtol(char*, char**, int);
extern	ulong	strtoul(char*, char**, int);
extern	long	end;

/*
 * Syscall data structures
 */

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

typedef struct Qid	Qid;
typedef struct Dir	Dir;
typedef struct Waitmsg	Waitmsg;

#define	ERRLEN	64
#define	DIRLEN	116
#define	NAMELEN	28

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
	int	pid;		/* of loved one */
	int	status;		/* unused; a placeholder */
	ulong	time[3];	/* of loved one */
	char	msg[ERRLEN];
};
