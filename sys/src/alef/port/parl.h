/* misc constants */
enum
{
	Eof		= -1,
	False		= 0,
	True		= 1,
	Strsize		= 512,
	Chunk		= 512*1024,
	NADDR		= 1,
	Sdepth		= 64,
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
	Node	*left;
	Node	*right;
	union {
		Node	*init;
		ulong	atvsafe;
	};
	Type	*t;
	Sym	*sym;
	Tinfo	*ti;
	int	srcline;
	union {
		long	pc;
		long	ival;
		float	fval;
		Node	*proto;
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

/* Storage classes */
enum
{
	Internal = 1,
	External,
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

	MARITH		= (MINT|MUINT|MSINT|MSUINT|MCHAR|MFLOAT),
	MUNSIGNED	= (MCHAR|MUINT|MSUINT|MIND),
	MINCDEC		= (MINT|MUINT|MSINT|MSUINT|MCHAR|MIND),
	MCOMPLEX	= (MADT|MUNION|MAGGREGATE),
	MCONVIND	= (MINT|MUINT|MIND)
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
	Nrops,
};

struct Type
{
	int	type;
	Sym	*tag;
	Sym	*sym;
	Type	*member;
	Type	*next;
	ulong	size;
	ulong	offset;
	uchar	align;
	int	class;
	union {
		int	nbuf;
		Node	*proto;
	};
};
#define ZeroT	((Type*)0)

struct Tinfo
{
	Type	*t;
	ulong	offset;
	Tinfo	*next;
	Tinfo	*dcllist;
	Sym	*s;
	int	block;
	int	class;
	char	ref;
};

enum
{
	Hashsize = 128,
};

struct Sym
{
	char	*name;
	int	lexval;
	Sym	*hash;
	Type	*ltype;
	Tinfo	*instance;
	int	sval;
	int	slot;
};

#define ZeroS	((Sym*)0)

#include "machdep.h"

struct Jmps
{
	Inst	*i;
	int	par;
	int	crit;
	Jmps	*next;
};
