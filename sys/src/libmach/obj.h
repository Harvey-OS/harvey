/*
 * obj.h -- defs for dealing with object files
 */

#pragma	lib	"libmach.a"

typedef struct	Prog	Prog;

typedef enum Kind		/* prog entries from intermediate that we care about */
{
	aNone,			/* we don't care about this prog */
	aName,			/* introduces a name */
	aText,			/* starts a function */
	aData,			/* references to a global object */
} Kind;

enum
{
	CHUNK	= 256,		/* number of Syms to allocate at once */
	NNAMES	= 50,
	MAXIS	= 8,		/* max length to determine if a file is a .? file */
	MAXOFF	= 0x7fffffff,	/* larger than any possible local offset */
};

#define UNKNOWN	'?'
#define islocal(t)	((t)=='a' || (t)=='p')

struct Prog		/* info from .$O files */
{
	Kind	kind;		/* what kind of symbol */
	char	type;		/* type of the symbol: ie, 'T', 'a', etc. */
	char	sym;		/* index of symbol's name */
	char	id[NNAME];	/* name for the symbol, if it introduces one */
};

extern struct Sym	*_sym;
extern char		*_filename,
			_symname[],
			_firstname[];
extern int		_global;
extern long		_nsym,
			_off,
			_symsize;
extern Biobuf		*_bin,
			_bout;

/* obj.c */
int		_nextar(void),
		_objsyms(int, int, void (*)(struct Sym *, long));
void		_offset(int, char, long),
		_assure(int);

/* [$OS].c */
int	_is2(char*),
	_is6(char*),
	_is8(char*),
	_isk(char*),
	_isv(char*),
	_isz(char*);
Prog	*_read2(Prog*),
	*_read6(Prog*),
	*_read8(Prog*),
	*_readk(Prog*),
	*_readv(Prog*),
	*_readz(Prog*);
