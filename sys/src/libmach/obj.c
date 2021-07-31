/*
 * obj.c
 * routines universal to all object files
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ar.h>
#include <mach.h>
#include "obj.h"

#define islocal(t)	((t)=='a' || (t)=='p')

enum
{
	CHUNK	= 256,		/* number of Syms to allocate at once */
	NNAMES	= 50,
	MAXIS	= 8,		/* max length to determine if a file is a .? file */
	MAXOFF	= 0x7fffffff,	/* larger than any possible local offset */
};

int	_is2(char*),		/* in [$OS].c */
	_is6(char*),
	_is8(char*),
	_isk(char*),
	_isv(char*),
	_isz(char*),
	_read2(Biobuf*, Prog*),
	_read6(Biobuf*, Prog*),
	_read8(Biobuf*, Prog*),
	_readk(Biobuf*, Prog*),
	_readv(Biobuf*, Prog*),
	_readz(Biobuf*, Prog*);

typedef struct Obj	Obj;

struct	Obj		/* functions to handle each intermediate (.$O) file */
{
	int	(*is)(char*);			/* test for each type of $O file */
	int	(*read)(Biobuf*, Prog*);	/* read for each type of $O file*/
};

static Obj	obj[] =
{			/* functions to identify and parse each type of obj */
	[Obj68020]	_is2, _read2,
	[ObjSparc]	_isk, _readk,
	[ObjMips]	_isv, _readv,
	[ObjHobbit]	_isz, _readz,
	[Obj386]	_is8, _read8,
	[Obj960]	_is6, _read6,
	[Maxobjtype]	0, 0
};

static	struct	Sym *sym;	/* base of symbol array */
static	int	names[NNAMES];	/* working set of active names */
static	int	local;		/* start of current local symbols in sym */
static	int	global;		/* start of current global symbols in sym */
static	long	nsym;		/* number of symbols */
static	int	have;		/* current size of symbol cache */

static	int	processprog(Prog*);	/* decode each symbol reference */
static	void	objlookup(int, char *, int );
static	int 	objupdate(int, int);
static	void	assure(int);

int
objtype(Biobuf *bp)
{
	int i;
	char buf[MAXIS];

	if(Bread(bp, buf, MAXIS) < MAXIS)
		return -1;
	Bseek(bp, -MAXIS, 1);
	for (i = 0; obj[i].is; i++) {
		if ((*obj[i].is)(buf))
			return i;
	}
	return -1;
}

int
isar(Biobuf *bp)
{
	int n;
	char magbuf[SARMAG];

	n = Bread(bp, magbuf, SARMAG);
	if(n == SARMAG && strncmp(magbuf, ARMAG, SARMAG) == 0)
		return 1;
	return 0;
}

/*
 * determine what kind of object file this is and process it.
 * return whether or not this was a recognized intermediate file.
 */
int
readobj(Biobuf *bp, int objtype)
{
	Prog p;

	if (objtype < 0 || objtype >= Maxobjtype)
		return 1;
	local = nsym;
	global = nsym;
	while ((*obj[objtype].read)(bp, &p))
		if (!processprog(&p))
			return 0;
	return 1;
}

int
readar(Biobuf *bp, int objtype, int end)
{
	Prog p;

	if (objtype < 0 || objtype >= Maxobjtype)
		return 1;
	local = nsym;
	global = nsym;
	while ((*obj[objtype].read)(bp, &p) && BOFFSET(bp) < end)
		if (!processprog(&p))
			return 0;
	return 1;
}

/*
 *	decode a symbol reference or definition
 */
static	int
processprog(Prog *p)
{
	if(p->kind == aNone)
		return 1;
	if(p->sym < 0 || p->sym >= NNAMES)
		return 0;
	switch(p->kind)
	{
	case aName:
		objlookup(p->sym, p->id, p->type);
		break;
	case aText:
		objupdate(p->sym, 'T');
		break;
	case aData:
		objupdate(p->sym, 'D');
		break;
	default:
		break;
	}
	return 1;
}

/*
 * find the entry for s in the symbol array.
 * make a new entry if it is not already there.
 */
static void
objlookup(int id, char *name, int type)
{
	int i;
	Sym *s;

	if (sym) {
		s = &sym[names[id]];
		i = islocal(type) ? local : global;
		if (names[id] >= i && strncmp(s->name, name, NNAME) == 0) {
			s->type = type;
			return;
		}
		for (s = &sym[i]; i < nsym; i++, s++) {
			if (strncmp(s->name, name, NNAME) == 0) {
				switch(s->type) {
				case 'T':
				case 'D':
				case 'U':
					if (type == 'U') {
						names[id] = i;
						return;
					}
					break;
				case 't':
				case 'd':
				case 'b':
					if (type == 'b') {
						names[id] = i;
						return;
					}
					break;
				case 'a':
				case 'p':
					if (type == 'a' || type == 'p') {
						names[id] = i;
						return;
					}
					break;
				default:
					break;
				}
			}
		}
	}
	names[id] = nsym++;
	assure(nsym);
	s = &sym[names[id]];

	memmove(s->name, name, NNAME);
	s->type = type;
	s->value = islocal(type) ? MAXOFF : 0;
	return;
}
/*
 *	return base of symbol storage and number of symbols
 */
Sym*
objbase(long *n)
{
	*n = nsym;
	return sym;
}
/*
 *	return i-th Symbol
 */
Sym*
objsym(int index)
{
	if (!sym)
		return 0;
	if (index < 0 || index >= nsym)
		return 0;
	return sym+index;
}

/*
 * update the offset information for a 'a' or 'p' symbol in an intermediate file
 */
void
_offset(int id, long off)
{
	Sym *s;
	
	s = &sym[names[id]];
	if (s->name[0] && islocal(s->type) && s->value > off)
		s->value = off;
}

/*
 * update the type of a global text or data symbol
 */
static int 
objupdate(int id, int type)
{
	Sym *s;

	s = &sym[names[id]];
	if (s->name[0])
		if (s->type == 'U') {
			s->type = type;
			return 1;
		} else if (s->type == 'b' || s->type == tolower(type)) {
			s->type = tolower(type);
			return 1;
		} else if (s->type == type)
			return 1;
	return 0;
}

/*
 * look for the next file in an archive
 */
int
nextar(Biobuf *bp, int offset, char *buf)
{
	struct ar_hdr a;
	int i, r;
	long arsize;

	if (offset&01)
		offset++;
	Bseek(bp, offset, 0);
	r = Bread(bp, &a, SAR_HDR);
	if(r != SAR_HDR)
		return 0;
	if(strncmp(a.fmag, ARFMAG, sizeof(a.fmag)))
		return -1;
	for(i=0; i<sizeof(a.name) && i<NNAME && a.name[i] != ' '; i++)
		buf[i] = a.name[i];
	buf[i] = 0;
	arsize = atol(a.size);
	if (arsize&1)
		arsize++;
	return arsize + SAR_HDR;
}

/*
 * make sure the symbol array can hold n elements;
 * expand it if it can't.
 */
static void
assure(int n)
{
	int old;

	if(n >= have){
		old = have;
		have = (n + CHUNK-1) &~ (CHUNK-1);
		sym = realloc(sym, have * sizeof(*sym));
		if(sym == 0){
			fprint(2, "%s: out of memory\n");
			exits("memory");
		}
		memset(sym + old, 0, (have - old) * sizeof(*sym));
	}
}

void
objreset(void)
{
	if (sym) {
		nsym = 0;
		memset(names, 0, sizeof names);
		memset(sym, 0, have * sizeof(*sym));
	}
}
