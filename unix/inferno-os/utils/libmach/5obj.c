/*
 * 5obj.c - identify and parse a arm object file
 */
#include <lib9.h>
#include <bio.h>
#include "5c/5.out.h"
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
_is5(char *s)
{
	return  s[0] == ANAME				/* ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}

int
_read5(Biobuf *bp, Prog *p)
{
	int as, n;
	Addr a;

	as = Bgetc(bp);			/* as */
	if(as < 0)
		return 0;
	p->kind = aNone;
	if(as == ANAME || as == ASIGNAME){
		if(as == ASIGNAME)
			skip(bp, 4);	/* signature */
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
	skip(bp, 6);		/* scond(1), reg(1), lineno(4) */
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
	long off;

	a.type = Bgetc(bp);	/* a.type */
	skip(bp,1);		/* reg */
	a.sym = Bgetc(bp);	/* sym index */
	a.name = Bgetc(bp);	/* sym type */
	switch(a.type){
	default:
	case D_NONE:
	case D_REG:
	case D_FREG:
	case D_PSR:
	case D_FPCR:
		break;
	case D_OREG:
	case D_CONST:
	case D_BRANCH:
	case D_SHIFT:
		off = Bgetc(bp);
		off |= Bgetc(bp) << 8;
		off |= Bgetc(bp) << 16;
		off |= Bgetc(bp) << 24;
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
