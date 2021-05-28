/*
 * functions (possibly) linked in, complete, from libc.
 */
#define nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define offsetof(s, m)	(ulong)(&(((s*)0)->m))
#define assert(x)	if(x){}else _assert("x")

/*
 * mem routines
 */
extern void* memset(void*, int, ulong);
extern int memcmp(void*, void*, ulong);
extern void* memmove(void*, void*, ulong);

/*
 * string routines
 */
extern int cistrcmp(char *, char *);
extern int cistrncmp(char *, char *, int);
extern char *strchr(char *, int);
extern int strcmp(char *, char *);
extern char* strecpy(char*, char*, char*);
extern long strlen(char*);
extern int strncmp(char *, char *, int);
extern char* strncpy(char*, char*, long);
extern char* strstr(char *, char *);
extern int tokenize(char*, char**, int);

/*
 * malloc
 */
extern void free(void*);
extern void* malloc(ulong);
extern void* mallocalign(ulong, ulong, long, ulong);
extern int mallocinit(void*, ulong);

/*
 * print routines
 */
typedef struct Fmt Fmt;
struct Fmt {
	uchar	runes;			/* output buffer is runes or chars? */
	void*	start;			/* of buffer */
	void*	to;			/* current place in the buffer */
	void*	stop;			/* end of the buffer; overwritten if flush fails */
	int	(*flush)(Fmt*);		/* called when to == stop */
	void*	farg;			/* to make flush a closure */
	int	nfmt;			/* num chars formatted so far */
	va_list	args;			/* args passed to dofmt */
	int	r;			/* % format Rune */
	int	width;
	int	prec;
	ulong	flags;
};

extern int print(char*, ...);
extern char* seprint(char*, char*, char*, ...);
extern char* vseprint(char*, char*, char*, va_list);

#pragma	varargck	argpos	print		1
#pragma	varargck	argpos	seprint		3

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
#pragma	varargck	type	"q"	char*
#pragma	varargck	type	"S"	Rune*
#pragma	varargck	type	"%"	void
#pragma	varargck	type	"p"	uintptr
#pragma	varargck	type	"p"	void*
#pragma	varargck	flag	','
#pragma	varargck	type	"E"	uchar*	/* eipfmt */
#pragma	varargck	type	"V"	uchar*	/* eipfmt */

extern int fmtinstall(int, int (*)(Fmt*));
extern int dofmt(Fmt*, char*);

/*
 * one-of-a-kind
 */
extern void _assert(char*);
extern uintptr getcallerpc(void*);
extern long strtol(char*, char**, int);
extern ulong strtoul(char*, char**, int);
extern	void	longjmp(jmp_buf, int);
extern	int	setjmp(jmp_buf);

extern char etext[], edata[], end[];
