/*
 * z - print symbols in a .z file
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "zc/z.out.h"
#include "obj.h"

typedef struct Addr	Addr;
struct Addr
{
	char	type;
	char	sym;
};
static Addr addr(void);
static char type2char(int);

int
_isz(char *s)
{
	return  s[0] == ANAME				/* ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}


Prog *
_readz(Prog *p)
{
	int i, as, c;
	Addr a;

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
	Bgetc(_bin);		/* lineno(low) */
	Bgetc(_bin);		/* lineno(high) */
	Bgetc(_bin);		/* lineno(lowhigh) */
	Bgetc(_bin);		/* lineno(highhigh) */
	a = addr();
	if(a.type != D_STATIC && a.type != D_EXTERN)
		p->kind = aNone;
	p->sym = a.sym;
	a = addr();
	if(a.type != D_CONST)
		p->kind = aNone;
	return p;
}

static Addr
addr(void)
{
	Addr a;
	int name, n, i;
	long off = 0;

	a.type = Bgetc(_bin);	/* type */
	Bgetc(_bin);		/* width */
	a.sym = Bgetc(_bin);	/* sym */
	name = a.type & ~D_INDIR;
	switch(name){
	case D_NONE:
		break;
	case D_CONST:
	case D_BRANCH:
	case D_ADDR:
	case D_REG:
	case D_EXTERN:
	case D_STATIC:
	case D_AUTO:
	case D_PARAM:
		off = Bgetc(_bin);
		off |= Bgetc(_bin) << 8;
		off |= Bgetc(_bin) << 16;
		off |= Bgetc(_bin) << 24;
		off = off<0 ? -off : off;
		break;
	case D_SCONST:
		n = Bgetc(_bin);
		for(i=0; i<n; i++)
			Bgetc(_bin);
		break;
	case D_FCONST:
		for(i=0; i<8; i++)
			Bgetc(_bin);
		break;
	}
	if(a.sym!=0 && (name==D_PARAM || name==D_AUTO))
		_offset(a.sym, type2char(name), off);
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
