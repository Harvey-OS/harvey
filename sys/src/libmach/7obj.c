/*
 * 7obj.c - identify and parse an arm64 object file
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
	return  (s[0]&0xFF) == (ANAME&0xFF)				/* ANAME */
		&& (s[1]&0xFF) == ANAME>>8
		&& s[2] == D_FILE			/* type */
		&& s[3] == 1				/* sym */
		&& s[4] == '<';				/* name of file */
}

int
_read7(Biobuf *bp, Prog *p)
{
	int as, n, c;
	Addr a;

	as = Bgetc(bp);			/* as (lo) */
	if(as < 0)
		return 0;
	c = Bgetc(bp);	/* as (hi) */
	if(c < 0)
		return 0;
	as |= ((c & 0xff) << 8);
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
	n = Bgetc(bp);		/* reg and flag */
	skip(bp, 4);		/* lineno(4) */
	a = addr(bp);
	if(n & 0x40)
		addr(bp);
	addr(bp);
	if(a.type != D_OREG || a.name != D_STATIC && a.name != D_EXTERN)
		p->kind = aNone;
	p->sym = a.sym;
	return 1;
}

static ulong
integ(Biobuf *bp)
{
	ulong v;

	v = Bgetc(bp);
	v |= Bgetc(bp) << 8;
	v |= Bgetc(bp) << 16;
	v |= Bgetc(bp) << 24;
	return v;
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
	case D_NONE:
	case D_REG:
	case D_SP:
	case D_FREG:
	case D_VREG:
	case D_COND:
		break;
	case D_OREG:
	case D_XPRE:
	case D_XPOST:
	case D_CONST:
	case D_BRANCH:
	case D_SHIFT:
	case D_EXTREG:
	case D_ROFF:
	case D_SPR:
		off = (long)integ(bp);
		if(off < 0)
			off = -off;
		if(a.sym && (a.name==D_PARAM || a.name==D_AUTO))
			_offset(a.sym, off);
		break;
	case D_DCONST:
		off = integ(bp);
		off |= (vlong)integ(bp) << 32;
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
