/*
 * functions (possibly) linked in, complete, from libc.
 */

/*
 * mem routines
 */
extern	void*	memset(void*, int, long);
extern	int	memcmp(void*, void*, long);
extern	void*	memmove(void*, void*, long);
extern	void*	memchr(void*, int, long);

/*
 * string routines
 */
extern	char*	strcat(char*, char*);
extern	char*	strchr(char*, char);
extern	char*	strrchr(char*, char);
extern	int	strcmp(char*, char*);
extern	char*	strcpy(char*, char*);
extern	char*	strncat(char*, char*, long);
extern	char*	strncpy(char*, char*, long);
extern	int	strncmp(char*, char*, long);
extern	long	strlen(char*);

/*
 * utf routines
 */
extern	char*	utfrune(char*, long);
extern	char*	utfrrune(char*, long);

/*
 *  math
 */
extern	int	abs(int);

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
extern	char*	donprint(char*, char*, char*, void*);
extern	int	fmtinstall(char, int(*)(Op*));
extern	int	sprint(char*, char*, ...);
extern	int	print(char*, ...);

#pragma	varargck	argpos	print	1
#pragma	varargck	argpos	sprint	2

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
#pragma	varargck	type	"S"	Rune*
#pragma	varargck	type	"r"	void
#pragma	varargck	type	"%"	void
#pragma	varargck	type	"p"	void*

/*
 * one-of-a-kind
 */
extern	void	qsort(void*, long, long, int (*)(void*, void*));
extern	char	end[];

extern	int	decrypt(void*, void*, int);
extern	int	encrypt(void*, void*, int);
extern	int	nrand(int);
extern	void	srand(int);
