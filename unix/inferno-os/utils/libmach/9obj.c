/*
 * 9obj.c - identify and parse a PowerPC-64 object file
 *	forsyth@terzarima.net
 */
#include <lib9.h>
#include <bio.h>
#include "mach.h"
#include "9c/9.out.h"
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
_is9(char *s)
{
	return  (s[0]&0377) == ANAME				/* ANAME */
		&& (s[1]&0377) == ANAME>>8
		&& s[2] == D_FILE			/* type */
		&& s[3] == 1				/* sym */
		&& s[4] == '<';				/* name of file */
}

int
_read9(Biobuf *bp, Prog *p)
{
	int as, n, c;
	Addr a;

	as = Bgetc(bp);			/* as(low) */
	if(as < 0)
		return 0;
	c = Bgetc(bp);		/* as(high) */
	if(c < 0)
		return 0;
	as |= ((c & 0xff) << 8);
	p->kind = aNone;
	p->sig = 0;
	if(as == ANAME || as == ASIGNAME){
		if(as == ASIGNAME){
			Bread(bp, &p->sig, 4);
			p->sig = beswal(p->sig);
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
	n = Bgetc(bp);	/* reg and flag */
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

static Addr
addr(Biobuf *bp)
{
	Addr a;
	vlong off;
	long l;

	a.type = Bgetc(bp);	/* a.type */
	skip(bp,1);		/* reg */
	a.sym = Bgetc(bp);	/* sym index */
	a.name = Bgetc(bp);	/* sym type */
	switch(a.type){
	default:
	case D_NONE: case D_REG: case D_FREG: case D_CREG:
	case D_FPSCR: case D_MSR:
		break;
	case D_SPR:
	case D_OREG:
	case D_CONST:
	case D_BRANCH:
	case D_DCONST:
	case D_DCR:
		l = Bgetc(bp);
		l |= Bgetc(bp) << 8;
		l |= Bgetc(bp) << 16;
		l |= Bgetc(bp) << 24;
		off = l;
		if(a.type == D_DCONST){
			l = Bgetc(bp);
			l |= Bgetc(bp) << 8;
			l |= Bgetc(bp) << 16;
			l |= Bgetc(bp) << 24;
			off = ((vlong)l << 32) | (off & 0xFFFFFFFF);
			a.type = D_CONST;	/* perhaps */
		}
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
