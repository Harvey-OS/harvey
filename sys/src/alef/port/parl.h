/* misc constants */
enum
{
	Eof		= -1,
	False		= 0,
	True		= 1,
	Strsize		= 512,
	Chunk		= 512*1024,
	NADDR		= (1<<0),
	NOUNBOX		= (1<<1),
	Sdepth		= 64,
	NDYHASH		= 32,
	NPHASH		= 32,
	Needed		= 0,
	Made,
};

typedef struct Node	Node;
typedef struct Sym	Sym;
typedef struct Type	Type;
typedef struct Tinfo	Tinfo;
typedef struct Jmps	Jmps;
typedef struct String	String;
typedef struct Hist	Hist;
typedef struct Tysym	Tysym;
typedef struct Stats	Stats;
typedef struct Tyrec	Tyrec;
typedef struct Altrec	Altrec;
typedef struct Dynt	Dynt;

struct Stats
{
	int	mem;
	int	nodes;
	int	types;
};

struct Tysym
{
	Sym	*s;
	Type	*t;
};

struct	Hist
{
	Hist*	link;
	char*	name;
	long	line;
	long	offset;
};
#define	HISTSZ	20
#define H	((Hist*)0)

struct Node
{
	char	type;
	char	islval;
	char	sun;
	char	reg;
	Node*	left;
	Node*	right;
	Node*	vlval;
	union {
		Node*	init;
		Node*	poly;
		Node*	link;
		ulong	atvsafe;
		char	ireg;
	};
	Type*	t;
	Sym*	sym;
	Tinfo*	ti;
	int	srcline;
	union {
		long	pc;
		long	ival;
		float	fval;
		Node*	proto;
	};
};
#define	ZeroN	((Node*)0)

struct String
{
	Node	n;
	int	len;
	String	*next;
	char	*string;
};

struct Dynt
{
	ulong	sig;
	Type*	t;
	Node*	n;
	Dynt*	hash;
};

/* Storage classes */
enum
{
	Internal = 1,
	External,
	Global,
	Parameter,
	Automatic,
	Argument,
	Private,
	Dfile,			/* Fake for output */
	Adtdeflt,
	Newtype,		/* Fake - always put last */
};

enum
{
	TXXX,
	TINT,
	TUINT,
	TSINT,
	TSUINT,
	TCHAR,
	TFLOAT,
	TIND,
	TCHANNEL,
	TARRAY,
	TAGGREGATE,
	TUNION,
	TFUNC,
	TVOID,
	TADT,
	TPOLY,
	Ntype,
};

enum
{
	MINT		= (1<<TINT),
	MUINT		= (1<<TUINT),
	MSINT		= (1<<TSINT),
	MSUINT		= (1<<TSUINT),
	MCHAR		= (1<<TCHAR),
	MFLOAT		= (1<<TFLOAT),
	MIND		= (1<<TIND),
	MCHANNEL	= (1<<TCHANNEL),
	MARRAY		= (1<<TARRAY),
	MAGGREGATE	= (1<<TAGGREGATE),
	MUNION		= (1<<TUNION),
	MFUNC		= (1<<TFUNC),
	MVOID		= (1<<TVOID),
	MADT		= (1<<TADT),
	MPOLY		= (1<<TPOLY),

	MARITH		= (MINT|MUINT|MSINT|MSUINT|MCHAR|MFLOAT),
	MUNSIGNED	= (MCHAR|MUINT|MSUINT|MIND),
	MBOOL		= (MARITH|MIND),
	MSCALAR		= (MARITH|MIND),
	MINCDEC		= (MINT|MUINT|MSINT|MSUINT|MCHAR|MIND),
	MCOMPLEX	= (MADT|MUNION|MAGGREGATE),
	MCONVIND	= (MINT|MUINT|MIND),
	MINTEGRAL	= (MINT|MUINT|MSINT|MSUINT|MCHAR),
};

enum
{
	OADD,
	OSUB,
	OMUL,
	OMOD,
	ODIV,
	OIND,
	OCAST,
	OADDR,
	OASGN,
	OARRAY,
	OCALL,
	OCAND,
	OCASE,
	OCONST,
	OCONV,
	OCOR,
	ODOT,
	OELSE,
	OEQ,
	OFOR,
	OFUNC,
	OGEQ,
	OGT,
	OIF,
	OLAND,
	OLEQ,
	OLIST,
	OLOR,
	OLSH,
	OLT,
	OAGDECL,
	ONAME,
	ONEQ,
	ONOT,
	OPAR,
	ORECV,
	ORET,
	ORSH,
	OSEND,
	OSTORAGE,
	OSWITCH,
	OUNDECL,
	OWHILE,
	OXOR,
	OPROTO,
	OVARARG,
	ODEFAULT,
	OCONT,
	OBREAK,
	OREGISTER,
	OINDREG,
	OARSH,
	OALSH,
	OJMP,
	OLO,
	OHI,
	OLOS,
	OHIS,
	ODWHILE,
	OPROCESS,
	OTASK,
	OSETDECL,
	OADDEQ,
	OSUBEQ,
	OMULEQ,
	ODIVEQ,
	OMODEQ,
	ORSHEQ,
	OLSHEQ,
	OANDEQ,
	OOREQ,
	OXOREQ,
	OTYPE,
	OLABEL,
	OGOTO,
	OPINC,
	OPDEC,
	OEINC,
	OEDEC,
	OADTDECL,
	OADTARG,
	OSELECT,
	OALLOC,
	OCSND,
	OCRCV,
	OILIST,
	OINDEX,
	OUALLOC,
	OCHECK,
	ORAISE,
	ORESCUE,
	OTCHKED,
	OBLOCK,
	OLBLOCK,
	OBECOME,
	OITER,
	OXEROX,
	OVASGN,
	Nrops,
};

struct Type
{
	uchar	type;
	uchar	poly;
	uchar	align;
	uchar	class;
	Sym*	tag;
	Sym*	sym;
	Type*	member;
	Type*	variant;
	Type*	param;
	Type*	next;
	Type*	subst;
	int	size;
	int	offset;
	union {
		int	nbuf;
		Node*	proto;
		Node*	dcltree;
	};
};
#define ZeroT	((Type*)0)

struct Tinfo
{
	Type*	t;
	long	offset;
	Tinfo*	next;
	Tinfo*	dcllist;
	Sym*	s;
	int	block;
	int	class;
	char	ref;
};

struct Altrec
{
	Tyrec*	r;
	Altrec*	prev;
};

struct Tyrec
{
	Type*	rcv;
	Type*	vch;
	Tyrec*	next;
};

enum
{
	Hashsize = 128,
};

struct Sym
{
	char*	name;
	int	lexval;
	Sym*	hash;
	Type*	ltype;
	Tinfo*	instance;
	int	sval;
	int	slot;
};

#define ZeroS	((Sym*)0)

#include "machdep.h"

struct Jmps
{
	Inst*	i;
	int	par;
	int	crit;
	Jmps*	next;
};
