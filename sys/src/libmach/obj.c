/*
 * obj.c
 * routines universal to all object files
 */
#include <u.h>
#include <libc.h>
#include <ar.h>
#include <a.out.h>
#include <bio.h>
#include "obj.h"
#include <ctype.h>

typedef struct Obj	Obj;

struct	Obj
{					/* functions to handle each intermediate (.$O) file */
	int	(*is)(char *);		/* test for a particular intermediate file */
	Prog	*(*read)(Prog *);	/* read a Prog record from the intermediate */
};

Obj	obj[] =
{					/* functions for all the intermeidate we know */
	_is2, _read2,
	_isk, _readk,
	_isv, _readv,
	_isz, _readz,
	_is8, _read8,
	_is6, _read6,
	0, 0
};

static int	names[NNAMES];		/* working set of active names */
static	char	arnamebuf[NNAME];
static	int	have;			/* number of element in sym */
static	int	local;			/* start of current local symbols in sym */
static long	nextoff;		/* offset of next element of archive */

long	_nsym;			/* number of symbols defined */
int	_global;		/* start of current global symbols in sym */
long	_off;			/* current offset in ar file */
long	_symsize;		/* size of the symbol file */

static void	readobj(Obj *, int),
		update(int, char);
static Sym	*lookup(char, char *, char);

/*
 * determine what kind of object file this is and process it.
 * return whether or not this was a recognized intermediate file.
 */
int
_objsyms(int isar, int reset, void (*process)(Sym *, long))
{
	char buf[MAXIS];
	static Obj *lasto = 0;
	Obj *o;

	if(Bread(_bin, buf, MAXIS) < MAXIS){
		fprint(2, "%s: file %s too short\n", argv0, _filename);
		return 0;
	}
	Bseek(_bin, -MAXIS, 1);
	for(o = obj; o->is; o++)
		if((*o->is)(buf)){
			if(reset){
				_nsym = 0;
				memset(names, 0, sizeof names);
				memset(_sym, 0, have * sizeof(*_sym));
			}else
			if(o != lasto)
				return 0;
			readobj(o, isar);
			(*process)(_sym, _nsym);
			lasto = o;
			return 1;
		}
	return 0;
}

/*
 * read and process the intermediate file
 */
static void
readobj(Obj *o, int isar)
{
	Prog p;

	local = _global = _nsym;
	while((*o->read)(&p)){
		if(isar && BOFFSET(_bin) >= nextoff)
			return;
		if(p.kind == aNone)
			continue;
		if(p.sym < 0 || p.sym >= NNAMES){
			fprint(2, "%s: too many syms in %s\n", argv0, _filename);
			fprint(2, "	%d with max=%d\n", p.sym, NNAMES);
			return;
		}
		switch(p.kind){
		case aName:
			lookup(p.sym, p.id, p.type);
			break;
		case aText:
			local = _nsym;
			update(p.sym, 'T');
			break;
		case aData:
			update(p.sym, 'D');
			break;
		}
	}
}

/*
 * find the entry for s in the symbol array.
 * make a new entry if it is not already there.
 */
static Sym *
lookup(char id, char *name, char type)
{
	int i;
	Sym *s;

	s = &_sym[names[id]];
	i = islocal(type) ? local : _global;
	if (names[id] >= i && strncmp(s->name, name, NNAME) == 0) {
		s->type = type;
		return s;
	}
	for (s = &_sym[i]; i < _nsym; i++, s++)
		if (strncmp(s->name, name, NNAME) == 0)
			switch(s->type) {
			case 'T':
			case 'D':
			case 'U':
				if (type == 'U') {
					names[id] = i;
					return s;
				}
				break;
			case 't':
			case 'd':
			case 'b':
				if (type == 'b') {
					names[id] = i;
					return s;
				}
				break;
			case 'a':
			case 'p':
				if (type == 'a' || type == 'p') {
					names[id] = i;
					return s;
				}
				break;
			default:
				break;
			}
	names[id] = _nsym++;
	_assure(_nsym);
	s = &_sym[names[id]];

	memmove(s->name, name, NNAME);
	s->type = type;
	s->value = islocal(type) ? MAXOFF : 0;
	return s;
}

/*
 * update the offset information for a 'a' or 'p' symbol in an intermediate file
 */
void
_offset(int id, char t, long off)
{
	Sym *s;
	
	USED(t);
	s = &_sym[names[id]];
	if (s->name[0] && islocal(s->type) && s->value > off)
		s->value = off;
}

/*
 * update the type of a global text or data symbol
 */
static void
update(int id, char type)
{
	Sym *s;

	s = &_sym[names[id]];
	if (s->name[0])
		if (s->type == 'U') {
			s->type = type;
			return;
		} else if (s->type == 'b' || s->type == tolower(type)) {
			s->type = tolower(type);
			return;
		} else if (s->type == type)
			return;
	fprint(2, "%s: unknown symbol reference: \"%s\" \n",
			argv0, (s->name[0]==0 ? "null" : s->name));
	exits("bad file");
}

/*
 * look for the next file in an archive
 */
int
_nextar(void)
{
	struct ar_hdr a;
	int i, r;
	long arsize;
again:
	if(_off == 0){
		nextoff = SARMAG;
		memset(_firstname, 0, NNAME);
	}
	if(nextoff & 1)
		nextoff++;
	_off = nextoff;
	Bseek(_bin, _off, 0);
	r = Bread(_bin, &a, SAR_HDR);
	if(r != SAR_HDR)
		return 0;
	if(strncmp(a.fmag, ARFMAG, sizeof(a.fmag))){
		fprint(2, "%s: phase error on ar header %ld\n", argv0, _off);
		return 0;
	}
	_filename = arnamebuf;
	for(i=0; i<sizeof(a.name) && i<NNAME; i++){
		r = a.name[i];
		if(r == ' ')
			break;
		_filename[i] = r;
	}
	_filename[i] = 0;
	arsize = atol(a.size);
	nextoff += arsize + SAR_HDR;
	if(_off == SARMAG)
		strncpy(_firstname, _filename, NNAME - 1);
	if(strcmp(_filename, _symname) == 0){
		_symsize += arsize + SAR_HDR;
		if(_symsize & 1)
			_symsize++;
		goto again;
	}
	return 1;
}

/*
 * make sure the symbol array can hold n elements;
 * expand it if it can't.
 */
void
_assure(int n)
{
	int old;

	if(n >= have){
		old = have;
		have = (n + CHUNK-1) &~ (CHUNK-1);
		_sym = realloc(_sym, have * sizeof(*_sym));
		if(_sym == 0){
			fprint(2, "%s: out of memory\n");
			exits("memory");
		}
		memset(_sym + old, 0, (have - old) * sizeof(*_sym));
	}
}
