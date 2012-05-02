/*
 * obj.h -- defs for dealing with object files
 */

/* things in plan 9 not in plan9ports */
enum {
				/* object file types */
	Obj68020 = 0,		/* .2 */
	ObjSparc,		/* .k */
	ObjMips,		/* .v */
	Obj386,			/* .8 */
	Obj960,			/* retired */
	Obj3210,		/* retired */
	ObjMips2,		/* .4 */
	Obj29000,		/* retired */
	ObjArm,			/* .5 */
	ObjPower,		/* .q */
	ObjMips2le,		/* .0 */
	ObjAlpha,		/* .7 */
	ObjSparc64,		/* .u */
	ObjAmd64,		/* .6 */
	ObjSpim,		/* .0 */
	ObjPower64,		/* .9 */
	Maxobjtype,
};

typedef struct  Sym     Sym;
struct  Sym
{
        vlong   value;
        uint    sig;
        char    type;
        char    *name;
};

#define islocal(t)	((t)=='a' || (t)=='p')

/* Back to you ... */
typedef enum Kind		/* variable defs and references in obj */
{
	aNone,			/* we don't care about this prog */
	aName,			/* introduces a name */
	aText,			/* starts a function */
	aData,			/* references to a global object */
} Kind;

typedef struct	Prog	Prog;

struct Prog		/* info from .$O files */
{
	Kind	kind;		/* what kind of symbol */
	char	type;		/* type of the symbol: ie, 'T', 'a', etc. */
	char	sym;		/* index of symbol's name */
	char	*id;		/* name for the symbol, if it introduces one */
	uint	sig;		/* type signature for symbol */
};

#define UNKNOWN	'?'
void		_offset(int, vlong);
