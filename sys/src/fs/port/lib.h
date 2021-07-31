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
extern	char*	utfrrune(char*, long);
extern	int	utflen(char*);
extern	int	runelen(long);

/*
 *  math
 */
extern	int	abs(int);

/*
 * print routines
 */
typedef struct Fmt	Fmt;
typedef int (*Fmts)(Fmt*);
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

extern	int	print(char*, ...);
extern	char*	vseprint(char*, char*, char*, va_list);
extern	int	snprint(char*, int, char*, ...);
extern	int	sprint(char*, char*, ...);

extern	int	fmtinstall(int c, int (*f)(Fmt*));
extern	int	quotefmtinstall(void);
extern	int	fmtit(Fmt *f, char *fmt, ...);
extern	int	fmtstrcpy(Fmt *f, char *s);

#pragma	varargck	argpos	fmtit	2
#pragma	varargck	argpos	print	1
#pragma	varargck	argpos	snprint	3
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

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */
