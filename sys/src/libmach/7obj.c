/*
 * 7obj.c - identify and parse an alpha object file
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "7c/7.out.h"
#include "obj.h"

typedef struct Addr	Addr;
struct Addr
{
	char	type;
	char	sym;
	char	name;
};
static Addr addr(Biobuf*);
static char type2char(int);
static void skip(Biobuf*, int);

int
_is7(char *s)
{
	return  s[0] == ANAME				/* ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}

int
_read7(Biobuf *bp, Prog *p)
{
	int as, n;
	Addr a;

	as = Bgetc(bp);			/* as */
	if(as < 0)
		return 0;
	p->kind = aNone;
	p->sig = 0;
	if(as == ANAME || as == ASIGNAME){
		if(as == ASIGNAME){
			Bread(bp, &p->sig, 4);
			p->sig = leswal(p->sig);
		}
		p->kind = aName;
		p->type = type2char(Bgetc(bp));		/* type */
		p->sym = Bgetc(bp);			/* sym */
		n = 0;
		for(;;) {
			as = Bgetc(bp);
			if(as < 0)
				return 0;
			n++;
			if(as == 0)
				break;
		}
		p->id = malloc(n);
		if(p->id == 0)
			return 0;
		Bseek(bp, -n, 1);
		if(Bread(bp, p->id, n) != n)
			return 0;
		return 1;
	}
	if(as == ATEXT)
		p->kind = aText;
	else if(as == AGLOBL)
		p->kind = aData;
	skip(bp, 5);		/* reg(1), lineno(4) */
	a = addr(bp);
	addr(bp);
	if(a.type != D_OREG || a.name != D_STATIC && a.name != D_EXTERN)
		p->kind = aNone;
	p->sym = a.sym;
	return 1;
}

static Addr
addr(Biobuf *bp)
{
	Addr a;
	vlong off;

	a.type = Bgetc(bp);	/* a.type */
	skip(bp,1);		/* reg */
	a.sym = Bgetc(bp);	/* sym index */
	a.name = Bgetc(bp);	/* sym type */
	switch(a.type){
	default:
	case D_NONE: case D_REG: case D_FREG: case D_PREG:
	case D_FCREG: case D_PCC:
		break;
	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		off = (uvlong)Bgetc(bp);
		off |= (uvlong)Bgetc(bp) << 8;
		off |= (uvlong)Bgetc(bp) << 16;
		off |= (uvlong)Bgetc(bp) << 24;
		off |= (uvlong)Bgetc(bp) << 32;
		off |= (uvlong)Bgetc(bp) << 40;
		off |= (uvlong)Bgetc(bp) << 48;
		off |= (uvlong)Bgetc(bp) << 56;
		if(off < 0)
			off = -off;
		if(a.sym && (a.name==D_PARAM || a.name==D_AUTO))
			_offset(a.sym, off);
		break;
	case D_SCONST:
		skip(bp, NSNAME);
		break;
	case D_FCONST:
		skip(bp, 8);
		break;
	}
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

static void
skip(Biobuf *bp, int n)
{
	while (n-- > 0)
		Bgetc(bp);
}
