/* acid.h */
enum
{
	Eof		= -1,
	Strsize		= 1024,
	Hashsize	= 128,
	Maxarg		= 10,
	NFD		= 100,
	Maxproc		= 50,
	Maxval		= 10,
};

typedef struct Node	Node;
typedef struct String	String;
typedef struct Lsym	Lsym;
typedef struct List	List;
typedef struct Store	Store;
typedef struct Gc	Gc;
typedef struct Strc	Strc;
typedef struct Rplace	Rplace;
typedef struct Ptab	Ptab;
typedef struct Value	Value;
typedef struct Type	Type;

Extern int	text;
Extern Fhdr	fhdr;
Extern int	line;
Extern Biobuf	*bin;
Extern Biobuf	*bout;
Extern char	symbol[Strsize];
Extern int	interactive;
Extern Node	*code;
Extern int	na;
Extern int	wtflag;
Extern Map	*cormap;
Extern Map	*symmap;
Extern Machdata	*machdata;
Extern ulong	dot;
Extern ulong	dotinc;
Extern int	xprint;
Extern char	asmbuf[Strsize];
Extern Lsym	*hash[Hashsize];
Extern long	dogc;
Extern Rplace	*ret;
Extern char	*filename;
Extern char	*aout;
Extern int	gotint;
Extern int	flen;
Extern Gc	*gcl;
Extern int	stacked;
Extern jmp_buf	err;

enum
{
	TINT,
	TFLOAT,
	TSTRING,
	TLIST,
};

struct Type
{
	Type	*next;
	Type	*down;
	int	offset;
	char	fmt;
	char	type[32];
	char	name[32];
};

struct Ptab
{
	int	pid;
	int	ctl;
};
Extern Ptab	ptab[Maxproc];

struct Rplace
{
	jmp_buf	rlab;
	Node	*val;
	Lsym	*local;
	Lsym	**tail;
};

struct Strc		/* Rock to hide things under to communicate with */
{			/* machdata routines */
	ulong	pc;
	ulong	sp;
	List	*l;
	ulong	cause;
	char	*excep;
};
Extern Strc strc;

struct Gc
{
	char	gcmark;
	Gc	*gclink;
};

struct Store
{
	char	fmt;
	Type	*comt;
	union {
		int	ival;
		double	fval;
		String	*string;
		List	*l;
	};
};

struct List
{
	Gc;
	List	*next;
	char	type;
	Store;
};

struct Value
{
	char	set;
	char	type;
	Store;
	Value	*pop;
	Lsym	*scope;
	Rplace	*ret;
};

struct Lsym
{
	char	*name;
	int	lexval;
	Lsym	*hash;
	Value	*v;
	Type	*lt;
	Node	*proc;
	void	(*builtin)(Node*, Node*);
};

struct Node
{
	Gc;
	char	op;
	char	type;
	Node	*left;
	Node	*right;
	Lsym	*sym;
	Store;
};
#define ZN	(Node*)0

struct String
{
	Gc;
	char	*string;
	int	len;
};

List*	addlist(List*, List*);
List*	al(int);
Node*	an(int, Node*, Node*);
void	append(Node*, Node*, Node*);
int	bool(Node*);
void	build(Node*);
void	call(char*, Node*, Node*, Node*, Node*);
void	catcher(void*, char*);
void	checkqid(int, int);
void	cmd(void);
Node*	con(int);
List*	construct(Node*);
void	ctrace(int);
void	decl(Lsym*, Lsym*);
void	deinstall(int);
void	delete(List*, int n, Node*);
void	dodot(Node*, Node*);
void	dostop(int);
void	dprint(char*, ...);
Lsym*	enter(char*, int);
void	error(char*, ...);
void	execute(Node*);
void	expr(Node*, Node*);
void	fatal(char*, ...);
ulong	findframe(ulong);
void	flatten(Node**, Node*);
int	get1(Map*, ulong, int, uchar*, int);
int	get2(Map*, ulong, int, ushort*);
int	get4(Map*, ulong, int, long*);
void*	gmalloc(long);
char*	ieeedtos(char*, ulong, ulong);
char*	ieeeftos(char*, ulong);
void	indir(Map*, ulong, char, Node*);
void	install(int);
void	installbuiltin(void);
void	kinit(void);
int	listcmp(List*, List*);
List*	listlocals(Symbol*, ulong);
List*	listparams(Symbol*, ulong);
List*	listvar(char*, long);
int	loadmodule(char*);
void	localaddr(Lsym*, Lsym*, Node*);
Lsym*	look(char*);
void	ltag(char*);
void	machinit(void);
Lsym*	mkvar(char*);
void	msg(int, char*);
void	notes(int);
int	nproc(char**);
void	nthelem(List*, int, Node*);
int	numsym(char);
void	pcode(Node*, int);
void	pexpr(Node*);
void	pstr(String*);
void	psymoff(ulong, int, char*);
int	put1(Map*, ulong, int, uchar*, int);
int	put2(Map*, ulong, int, ushort);
int	put4(Map*, ulong, int, long);
ulong	raddr(char*);
void	readtext(char*);
long	rget(char*);
int	scmp(String*, String*);
void	sproc(int);
String*	stradd(String*, String*);
String*	strnode(char*);
void	unwind(void);
void	varreg(void);
void	varsym(void);
void	whatis(Lsym*);
void	windir(Map *m, Node*, Node*, Node*);
void	yyerror(char*, ...);
int	yylex(void);
int	yyparse(void);
void	gc(void);

enum
{
	ONAME,
	OCONST,
	OMUL,
	ODIV,
	OMOD,
	OADD,
	OSUB,
	ORSH,
	OLSH,
	OLT,
	OGT,
	OLEQ,
	OGEQ,
	OEQ,
	ONEQ,
	OLAND,
	OXOR,
	OLOR,
	OCAND,
	OCOR,
	OASGN,
	OINDM,
	OEDEC,
	OEINC,
	OPINC,
	OPDEC,
	ONOT,
	OIF,
	ODO,
	OLIST,
	OCALL,
	OCTRUCT,
	OWHILE,
	OELSE,
	OHEAD,
	OTAIL,
	OAPPEND,
	ORET,
	OINDEX,
	OINDC,
	ODOT,
	OLOCAL,
	OFRAME,
	OCOMPLEX,
	ODELETE,
};
