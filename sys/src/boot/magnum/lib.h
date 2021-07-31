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
extern	int	strcmp(char*, char*);
extern	char	*strcpy(char*, char*);
extern	char	*strncat(char*, char*, long);
extern	char	*strncpy(char*, char*, long);
extern	int	strncmp(char*, char*, long);
extern	long	strlen(char*);

/*
 * print routines
 */

#define	FUNSIGN	4
#define	FSHORT	2
#define	FLONG	1

typedef struct Op	Op;
struct Op
{
	char	*p;
	char	*ep;
	void	*argp;
	int	f1;
	int	f2;
	int	f3;
};
extern	void	strconv(char*, Op*, int, int);
extern	int	numbconv(Op*, int);
extern	char	*doprint(char*, char*, char*, void*);
extern	int	fmtinstall(char, int (*)(Op*));
extern	int	sprint(char*, char*, ...);
extern	int	print(char*, ...);
extern  int	atoi(char *);
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
	Length;
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
