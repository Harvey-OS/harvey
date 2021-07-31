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
 * random number
 */
extern	void	srand(long);
extern	int	nrand(int);

/*
 * print routines
 */

#define	FLONG	(1<<0)
#define	FSHORT	(1<<1)
#define	FUNSIGN	(1<<2)

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
extern	int	fmtinstall(char, int(*)(Op*));
extern	int	sprint(char*, char*, ...);
extern	int	print(char*, ...);

/*
 * one-of-a-kind
 */
#define DESKEYLEN	7
extern	long	strtol(char*, char**, int);
extern	ulong	strtoul(char*, char**, int);
extern	long	atol(char*);
extern	void	qsort(void*, long, long, int (*)(void*, void*));
extern	int	encrypt(void*, void*, int);
extern	int	decrypt(void*, void*, int);
extern	long	end;

/*
 * syscalls needed
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

extern	void	abort(void);
extern	long	alarm(ulong);
extern	int	close(int);
extern	int	create(char*, int, ulong);
extern	int	mount(int, char*, int, char*, char*);
extern	int	open(char*, int);
extern	int	pipe(int*);
extern	long	read(int, void*, long);
extern	int	remove(char*);
extern	int	rfork(int);
extern	long	seek(int, long, int);
extern	int	segattach(int, char*, void*, ulong);
extern	int	segbrk(void*, void*);
extern	int	sleep(long);
extern	long	write(int, void*, long);
extern	void	exits(char*);
extern	int	notify(void(*)(void*, char*));
extern	int	noted(int);

extern	int	getpid(void);
extern	int	fstat(int, char*);

/*
 * argument processing
 */
#define	ARGBEGIN	for(argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt, _argc;\
				_args = &argv[0][1];\
				SET(_argc);\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				while(*_args) switch(_argc=*_args++)
#define	ARGEND		USED(_argc);}
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc
extern char *argv0;
