#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../ld/elf.h"

typedef vlong int64;

/*
 * basic types in all loaders
 */

typedef	struct	Adr	Adr;
typedef	struct	Auto	Auto;
typedef	struct	Count	Count;
typedef	struct	Ieee	Ieee;
typedef	struct	Prog	Prog;
typedef	struct	Sym	Sym;

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Auto
{
	Sym*	asym;
	Auto*	link;
	vlong	aoffset;
	short	type;
};

struct	Count
{
	long	count;
	long	outof;
};

enum
{

	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MAXIO		= 16*1024,
	MAXHIST		= 20,	/* limit of path elements for history symbols */
};

#define SIGNINTERN	(1729*325*1729)	/* signature of internal functions such as _div */

EXTERN union
{
	struct
	{
		uchar	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	char	debug[128];
EXTERN	char	fnuxi4[4];
EXTERN	char	fnuxi8[8];
EXTERN	Sym*	hash[NHASH];
EXTERN	Sym*	histfrog[MAXHIST];
EXTERN	int	histfrogp;
EXTERN	int	histgen;
EXTERN	char*	library[50];
EXTERN	char*	libraryobj[50];
EXTERN	int	libraryp;
EXTERN	int	xrefresolv;
EXTERN	char	inuxi1[1];
EXTERN	char	inuxi2[2];
EXTERN	char	inuxi4[4];
EXTERN	uchar	inuxi8[8];
EXTERN	char*	thestring;
EXTERN	char	thechar;

EXTERN	int	doexp, dlm;
EXTERN	int	imports, nimports;
EXTERN	int	exports, nexports;
EXTERN	char*	EXPTAB;
EXTERN	Prog	undefp;

#define	UP	(&undefp)

int	Sconv(Fmt*);
void	addhist(long, int);
void	addlib(char*);
void	addlibpath(char*);
void	addlibroot(void);
vlong	atolwhex(char*);
Prog*	brchain(Prog*);
Prog*	brloop(Prog*);
void	cflush(void);
void	ckoff(Sym*, long);
void	collapsefrog(Sym*);
void	cput(int);
void	diag(char*, ...);
void	errorexit(void);
double	cputime(void);
void	dodata(void);
void	export(void);
int	fileexists(char*);
int	find1(long, int);
char*	findlib(char*);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
long	hunkspace(void);
uchar*	readsome(int, uchar*, uchar*, uchar*, int);
void* halloc(usize);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	import(void);
int	isobjfile(char*);
void	loadlib(void);
Sym*	lookup(char*, int);
void	mkfwd(void);
void*	mysbrk(ulong);
void	nopstat(char*, Count*);
void	objfile(char*);
void	patch(void);
void	prasm(Prog*);
Prog*	prg(void);
void	readundefs(char*, int);
uchar*	readsome(int, uchar*, uchar*, uchar*, int);
void	readundefs(char*, int);
vlong	rnd(vlong, long);
void	strnput(char*, int);
void	undef(void);
void	undefsym(Sym*);
void	xdefine(char*, int, vlong);
void	xfol(Prog*);
void	zerosig(char*);

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"C"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1
