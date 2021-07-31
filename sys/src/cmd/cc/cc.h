#include <u.h>
#include <libc.h>
#include <ctype.h>

#pragma	lib	"../cc/cc.a$O"

typedef	struct	Node	Node;
typedef	struct	Sym	Sym;
typedef	struct	Type	Type;
typedef	struct	Decl	Decl;
typedef	struct	Io	Io;
typedef	struct	Hist	Hist;
typedef	struct	Ref	Ref;

#define	CNNAME		20
#define	FPCHIP		1
#define	NHUNK		50000L
#define	BUFSIZ		8192
#define	NSYMB		500
#define	NTYPE		11
#define	XTYPE		(NTYPE+6)
#define	NHASH		1024
#define	STRINGSZ	200
#define	HISTSZ		20
#define	NINCLUDE	10
#define YYMAXDEPTH	500

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

struct	Node
{
	union
	{
		struct
		{
			union
			{
				Node*	left;
				void*	label;
				int	reg;
			};
			union
			{
				Node*	right;
				long	offset;
			};
		};
		double	ud;	/* fp constant */
		char*	us;	/* character string */
		ushort*	rs;	/* rune string */
	};
	Sym*	sym;
	Type*	type;
	Ref*	ref;		/* try to put in l/r su */
	long	lineno;
	char	op;
	char	class;
	char	etype;
	char	complex;
	char	addable;
	char	scale;
};
#define	Z	((Node*)0)

struct	Ref
{
	Ref*	link;
	long	lineno;			/* usage lineno */
	union
	{
		long	dlineno;	/* def lineno if this is usage */
		Sym*	sym;		/* file name for CHELP */
	};
	char	class;			/* class of this ref */
};

struct	Sym
{
	Sym*	link;
	Type*	type;
	Type*	suetag;
	char*	macro;
	Ref*	ref;
	long	varlineno;
	long	offset;
	Node*	label;
	ushort	lexical;
	char	name[CNNAME];
	char	block;
	char	sueblock;
	char	class;
	char	sym;
	char	aused;
};
#define	S	((Sym*)0)

struct	Decl
{
	Decl*	link;
	Sym*	sym;
	Type*	type;
	long	varlineno;
	long	offset;
	short	val;
	char	class;
	char	block;
	char	aused;
};
#define	D	((Decl*)0)

struct	Type
{
	Sym*	sym;
	Sym*	tag;
	Type*	link;
	Type*	down;
	long	width;
	long	offset;
	long	lineno;
	char	shift;
	char	nbits;
	char	etype;
};
#define	T	((Type*)0)
#define	NODECL	((void(*)(int, Type*, Sym*))0)

struct
{
	char*	p;
	int	c;
} fi;

struct	Io
{
	Io*	link;
	char*	p;
	char	b[BUFSIZ];
	short	c;
	short	f;
};
#define	I	((Io*)0)

struct	Hist
{
	Hist*	link;
	char*	name;
	long	line;
	long	offset;
};
#define	H	((Hist*)0)
Hist*	hist;

enum
{
	DMARK,
	DAUTO,
	DSUE,
	DLABEL,
};
enum
{
	OXXX,
	OADD,	OADDR,	OAND,	OANDAND,OARRAY,	OAS,	OASADD,	OASAND,
	OASASHL,OASASHR,OASDIV,	OASHL,	OASHR,	OASLDIV,OASLMOD,OASLMUL,
	OASLSHR,OASMOD,	OASMUL,	OASOR,	OASSUB,	OASXOR,	OBIT,	OBREAK,
	OCASE,	OCAST,	OCOMMA,	OCOND,	OCONST,	OCONTINUE,ODIV,	ODOT,
	ODOTDOT,ODWHILE,OENUM,	OEQ,	OFOR,	OFUNC,	OGE,	OGOTO,
	OGT,	OHI,	OHS,	OIF,	OIND,	OINDREG,OINIT,	OLABEL,
	OLDIV,	OLE,	OLIST,	OLMOD,	OLMUL,	OLO,	OLS,	OLSHR,
	OLT,	OMOD,	OMUL,	ONAME,	ONE,	ONOT,	OOR,	OOROR,
	OPOSTDEC,OPOSTINC,OPREDEC,OPREINC,OPROTO,OREGISTER,ORETURN,OSET,
	OSIZE,	OSTRING,OLSTRING,OSTRUCT,OSUB,	OSWITCH,OUNION,	OUSED,
	OWHILE,	OXOR,	OEND
};
enum
{
	TXXX,
	TCHAR,	TUCHAR,	TSHORT,	TUSHORT,TLONG,	TULONG,	TVLONG,	TFLOAT,	TDOUBLE,
	TIND,	TFUNC,	TARRAY,	TVOID,
	TSTRUCT,TUNION,	TENUM,	TFILE,  TOLD,   TDOT,
};
enum
{
	CXXX,
	CAUTO,	CEXTERN,CGLOBL,CSTATIC,CLOCAL,CTYPEDEF,CPARAM,
	CSELEM,	CLABEL, CEXREG,
	/*
	 * used in ref structure
	 */
	CHELP, CMACRO, CMACARG, CLEXICAL, CPREPROC, CSUETAG,
	CENUM, CLAST,
};
enum
{
	BXXX		= 1L<<TXXX,
	BCHAR		= 1L<<TCHAR,
	BUCHAR		= 1L<<TUCHAR,
	BSHORT		= 1L<<TSHORT,
	BUSHORT		= 1L<<TUSHORT,
	BLONG		= 1L<<TLONG,
	BULONG		= 1L<<TULONG,
	BVLONG		= 1L<<TVLONG,
	BFLOAT		= 1L<<TFLOAT,
	BDOUBLE		= 1L<<TDOUBLE,
	BIND		= 1L<<TIND,
	BVOID		= 1L<<TVOID,
	BSTRUCT		= 1L<<TSTRUCT,
	BUNION		= 1L<<TUNION,
	BFUNC		= 1L<<TFUNC,

	BINTEGER	= BCHAR|BUCHAR|BSHORT|BUSHORT|BLONG|BULONG|BVLONG,
	BNUMBER		= BINTEGER|BFLOAT|BDOUBLE,

/* these can be overloaded with complex types */
	BUNSIGNED	= BFUNC,
	BINT		= BUNION,
	BSIGNED		= BSTRUCT,

	BAUTO		= 1L<<(TENUM+1),
	BEXTERN		= 1L<<(TENUM+2),
	BSTATIC		= 1L<<(TENUM+3),
	BTYPEDEF	= 1L<<(TENUM+4),
	BREGISTER	= 1L<<(TENUM+5),
	BCLASS		= BAUTO|BEXTERN|BSTATIC|BTYPEDEF|BREGISTER,
};

int	autobn;
long	autoffset;
int	blockno;
Decl*	dclstack;
char	debug[256];
Hist*	ehist;
long	firstbit;
Sym*	firstarg;
Type*	firstargtype;
Decl*	firstdcl;
int	fperror;
Sym*	hash[NHASH];
char*	hunk;
char*	include[NINCLUDE];
Io*	iofree;
Io*	ionext;
Io*	iostack;
long	lastbit;
char	lastclass;
Type*	lastdcl;
long	lastenum;
long	lastfield;
Type*	lasttype;
long	lineno;
int	lnstring;
long	maxenum;
long	nearln;
int	nerrors;
int	newflag;
long	nhunk;
int	ninclude;
Node*	nodproto;
char*	outfile;
int	peekc;
long	stkoff;
Type*	strf;
Type*	strl;
int	suround;
int	supad;
char	symb[NSYMB];
Sym*	symstring;
Type*	tfield;
int	thechar;
char*	thestring;
Type*	thisfn;
long	thunk;
Type*	tint;
Type*	tuint;
Type*	types[XTYPE];
char*	suedebug;
Node*	initlist;

extern	char	*onames[], *tnames[], *cnames[], *qnames[];
extern	char	typechl[], typechlp[], typelp[], typesu[], typefdv[];
extern	char	typeu[], typeaf[], typel[], typscalar[];
extern	char	comrel[], invrel[], logrel[], typec[], typeh[];
extern	char	tab[NTYPE][NTYPE];
extern	long	lcast[], ncast[], tadd[], tand[];
extern	long	targ[], tasadd[], tasign[], tcast[];
extern	long	tdot[], tfunct[], tindir[], tmul[];
extern	long	tnot[], trel[], tsub[];

/*
 *	compat
 */
int	mywait(void*);
int	mycreat(char*, int);
char*	myerrstr(int);
int	unix(void);

/*
 *	parser
 */
int	yyparse(void);
int	mpatof(char*, double*);
int	mpatol(char*, long*);

/*
 *	lex.c
 */
void	cinit(void);
void	errorexit(void);
int	filbuf(void);
int	getc(void);
long	getr(void);
int	getnsc(void);
Sym*	lookup(void);
void	main(int, char*[]);
void	newfile(char*, int);
void	newio(void);
void	pushio(void);
long	escchar(long, int, int);
Sym*	slookup(char*);
void	syminit(Sym*);
void	unget(int);
long	yylex(void);
int	Lconv(void*, Fconv*);
int	Tconv(void*, Fconv*);
int	FNconv(void*, Fconv*);
int	Oconv(void*, Fconv*);
int	Qconv(void*, Fconv*);
int	VBconv(void*, Fconv*);

/*
 * mac.c
 */
void	dodefine(char*);
void	domacro(void);
Sym*	getsym(void);
void	linehist(char*, int);
void	macdef(void);
void	macprag(void);
void	macend(void);
void	macexpand(Sym*, char*);
void	macif(int);
void	macinc(void);
void	maclin(void);
void	macund(void);

/*
 * dcl.c
 */
Node*	doinit(Sym*, Type*, long, Node*);
Type*	tcopy(Type*);
Node*	init1(Sym*, Type*, long, int);
Node*	newlist(Node*, Node*);
void	adecl(int, Type*, Sym*);
int	allign(Type*);
int	anyproto(Node*);
void	argmark(Node*, int);
void	dbgdecl(Sym*);
Node*	dcllabel(Sym*, int);
Node*	dodecl(void(*)(int, Type*, Sym*), int, Type*, Node*);
Sym*	mkstatic(Sym*);
void	doenum(Sym*, Node*);
void	dotag(Sym*, int, int);
void	edecl(int, Type*, Sym*);
Type*	fnproto(Node*);
Type*	fnproto1(Node*);
void	markdcl(void);
Type*	maxtype(long);
Type*	paramconv(Type*, int);
void	pdecl(int, Type*, Sym*);
Decl*	push(void);
Decl*	push1(Sym*);
void	revertdcl(void);
int	round(long, long);
int	rsametype(Type*, Type*, int);
int	sametype(Type*, Type*);
void	suallign(Type*);
void	tmerge(Type*, Sym*);
void	walkparam(Node*, int);
void	xdecl(int, Type*, Sym*);
void	dbgprint(Sym*, Type*);
Node*	contig(Sym*, Node*, long);

/*
 * com.c
 */
void	ccom(Node*);
void	complex(Node*);
int	tcom(Node*);
int	tcoma(Node*, Node*, Type*, int);
int	tcomd(Node*);
int	tcomo(Node*, int);
int	tcomx(Node*);
int	tlvalue(Node*);

/*
 * con.c
 */
void	acom(Node*);
void	acom1(long, Node*);
void	acom2(Node*, Type*);
int	acomcmp1(void*, void*);
int	acomcmp2(void*, void*);
int	addo(Node*);
void	evconst(Node*);
void	reftrace(void);

/*
 * sub.c
 */
void	arith(Node*, int);
Type*	dotsearch(Sym*, Type*, Node*);
long	dotoffset(Type*, Type*, Node*);
void	gethunk(void);
Node*	invert(Node*);
int	bitno(long);
void	makedot(Node*, Type*, long);
Node*	new(int, Node*, Node*);
Node*	new1(int, Node*, Node*);
int	nilcast(Type*, Type*);
int	nocast(Type*, Type*);
void	prtree(Node*, char*);
void	prtree1(Node*, int, int);
void	relcon(Node*, Node*);
int	relindex(int);
int	simplec(long);
Type*	simplet(long);
int	stcompat(Node*, Type*, Type*, long[]);
int	tcompat(Node*, Type*, Type*, long[]);
Type*	typ(int, Type*);
void	typeext(Type*, Node*);
int	ovflo(long, int);
int	side(Node*);
int	vconst(Node*);
void	diag(Node*, char*, ...);
void	warn(Node*, char*, ...);
void	yyerror(char*, ...);

/*
 * calls to machine depend part
 */
void	codgen(Node*, Node*);
void	gclean(void);
void	gextern(Sym*, Node*, long, long);
void	ginit(void);
long	outstring(char*, long);
long	outlstring(ushort*, long);
void	sextern(Sym*, Node*, long, long);
void	xcom(Node*);
long	exreg(Type*);
char*	xOconv(int);
long	castto(long, int);
int	endian(int);

extern	schar	ewidth[];
