/*
 * k - print symbols in a .k file
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "kc/k.out.h"
#include "obj.h"

typedef struct Addr	Addr;
struct Addr
{
	char	type;
	char	sym;
	char	name;
	ulong	off;
	char	sconst[NSNAME+1];
	uchar	fconst[8];
};
static Addr addr(void);
static char type2char(int);


int
_isk(char *s)
{
	return  s[0] == ANAME				/* ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}


Prog *
_readk(Prog *p)
{
	int i, as, c;
	Addr a;
	Addr b;

	as = Bgetc(_bin);			/* as */
	if(as < 0)
		return 0;
	p->kind = aNone;
	if(as == ANAME){
		p->kind = aName;
		p->type = type2char(Bgetc(_bin));		/* type */
		p->sym = Bgetc(_bin);			/* sym */
		c = Bgetc(_bin);
		for(i=0; i < NNAME && c > 0; i++){
			p->id[i] = c;
			c = Bgetc(_bin);
		}
		if(i < NNAME)
			p->id[i] = c;
		return p;
	}
	if(as == ATEXT)
		p->kind = aText;
	else if(as == AGLOBL)
		p->kind = aData;
	Bgetc(_bin);			/* reg */
	Bgetc(_bin);			/* lineno(low) */
	Bgetc(_bin);			/* lineno(high) */
	Bgetc(_bin);			/* lineno(lowhigh) */
	Bgetc(_bin);			/* lineno(highhigh) */
	a = addr();
	b = addr();
	if(a.type != D_OREG
	|| a.name != D_STATIC && a.name != D_EXTERN)
		p->kind = aNone;
	p->sym = a.sym;
	return p;
}

static Addr
addr(void)
{
	Addr a;
	int i;
	long off = 0;

	memset(&a, 0, sizeof(a));
	a.type = Bgetc(_bin);	/* a.type */
	Bgetc(_bin);	/* reg */
	a.sym = Bgetc(_bin);	/* sym index */
	a.name = Bgetc(_bin);	/* sym type */
	switch(a.type) {
	default:
		fprint(2, "%s: unknown type in addr %d\n", argv0, a.type);
		exits("type");
	case D_NONE: case D_REG: case D_FREG: case D_CREG: case D_PREG:
		break;
	case D_BRANCH:
	case D_OREG:
	case D_ASI:
	case D_CONST:
		off = Bgetc(_bin);
		off |= Bgetc(_bin) << 8;
		off |= Bgetc(_bin) << 16;
		off |= Bgetc(_bin) << 24;
		off = off<0 ? -off : off;
		break;
	case D_SCONST:
		for(i=0; i<NSNAME; i++)
			a.sconst[i] = Bgetc(_bin);
		break;
	case D_FCONST:
		for(i=0; i<8; i++)
			a.fconst[i] = Bgetc(_bin);
		break;
	}
	if(a.sym!=0 && (a.name==D_PARAM || a.name==D_AUTO))
		_offset(a.sym, type2char(a.name), off);
	a.off = off;
	return a;
}


static char
type2char(int t)
{
	switch(t){
	case D_EXTERN:		return 'U';
	case D_STATIC:		return 'b';
	case D_AUTO:		return 'a';
	case D_PARAM:		return 'p';
	default:		return UNKNOWN;
	}
}
