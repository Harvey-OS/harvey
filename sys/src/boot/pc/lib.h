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
extern	int	sprint(char*, char*, ...);
extern 	int	snprint(char*, int, char*, ...);
extern	int	fmtinstall(int, int (*)(Fmt*));

#pragma	varargck	argpos	fmtprint	2
#pragma	varargck	argpos	print		1
#pragma	varargck	argpos	seprint		3
#pragma	varargck	argpos	snprint		3
#pragma	varargck	argpos	sprint		2
#pragma varargck	type	"H" void*

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
#pragma	varargck	type	"E"	uchar*

#define PRINTSIZE	256

/*
 * one-of-a-kind
 */
extern	int	atoi(char*);
extern	ulong	getcallerpc(void*);
extern	long	strtol(char*, char**, int);
extern	ulong	strtoul(char*, char**, int);
extern	long	end;

#define	NAMELEN	28
