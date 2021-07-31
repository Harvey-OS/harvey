#pragma	lib	"../cc/cc.a$O"

typedef	struct	Sym	Sym;
typedef	struct	Ref	Ref;
typedef	struct	Gen	Gen;
typedef	struct	Io	Io;
typedef	struct	Hist	Hist;
typedef	struct	Gen2 	Gen2;

#define	FPCHIP		1
#define	NSYMB		500
#define	BUFSIZ		8192
#define	HISTSZ		20
#define	NINCLUDE	10
#define	NHUNK		10000
#define	EOF		(-1)
#define	IGN		(-2)
#define	GETC()		((--fi.c < 0)? filbuf(): *fi.p++ & 0xff)
#define	NHASH		503
#define	STRINGSZ	200
#define	NMACRO		10

#define	ALLOC(lhs, type)\
	while(nhunk < sizeof(type))\
		gethunk();\
	lhs = (type*)hunk;\
	nhunk -= sizeof(type);\
	hunk += sizeof(type);

#define	ALLOCN(lhs, len, n)\
	if(lhs+len != hunk || nhunk < n) {\
		while(nhunk <= len)\
			gethunk();\
		memmove(hunk, lhs, len);\
		lhs = hunk;\
		hunk += len;\
		nhunk -= len;\
	}\
	hunk += n;\
	nhunk -= n;

struct	Sym
{
	Sym*	link;
	Ref*	ref;
	char*	macro;
	long	value;
	ushort	type;
	char	*name;
	char	sym;
};
#define	S	((Sym*)0)

struct	Ref
{
	int	class;
};

struct
{
	char*	p;
	int	c;
} fi;

struct	Io
{
	Io*	link;
	char	b[BUFSIZ];
	char*	p;
	short	c;
	short	f;
};
#define	I	((Io*)0)

struct
{
	Sym*	sym;
	short	type;
} h[NSYM];

struct	Gen
{
	double	dval;
	char	sval[8];
	long	offset;
	Sym*	sym;
	uchar	type;
	uchar	index;
	uchar	scale;
};
struct	Gen2
{
	Gen	from;
	Gen	to;
	uchar	type;
	long	offset;
};

struct	Hist
{
	Hist*	link;
	char*	name;
	long	line;
	long	offset;
};
#define	H	((Hist*)0)

enum
{
	CLAST,
	CMACARG,
	CMACRO,
	CPREPROC,
};

char	debug[256];
Sym*	hash[NHASH];
char*	Dlist[30];
int	nDlist;
Hist*	ehist;
int	newflag;
Hist*	hist;
char*	hunk;
char*	include[NINCLUDE];
Io*	iofree;
Io*	ionext;
Io*	iostack;
long	lineno;
int	nerrors;
long	nhunk;
int	ninclude;
Gen	nullgen;
char*	outfile;
int	pass;
char*	pathname;
long	pc;
int	peekc;
int	sym;
char	symb[NSYMB];
int	thechar;
char*	thestring;
long	thunk;
Biobuf	obuf;

void	errorexit(void);
void	pushio(void);
void	newio(void);
void	newfile(char*, int);
Sym*	slookup(char*);
Sym*	lookup(void);
void	syminit(Sym*);
int	yylex(void);
int	getc(void);
int	getnsc(void);
void	unget(int);
int	escchar(int);
void	cinit(void);
void	checkscale(int);
void	pinit(char*);
void	cclean(void);
int	isreg(Gen*);
void	outcode(int, Gen2*);
void	outhist(void);
void	zaddr(Gen*, int);
void	zname(char*, int, int);
void	ieeedtod(Ieee*, double);
int	filbuf(void);
Sym*	getsym(void);
void	domacro(void);
void	macund(void);
void	macdef(void);
void	macexpand(Sym*, char*);
void	macinc(void);
void	macprag(void);
void	maclin(void);
void	macif(int);
void	macend(void);
void	dodefine(char*);
void	prfile(long);
void	linehist(char*, int);
void	gethunk(void);
void	yyerror(char*, ...);
int	yyparse(void);

/*
 *	compat
 */
int	mywait(void*);
int	mycreat(char*, int);
char*	myerrstr(int);
int	unix(void);
