/*
 * kobj.c - identify and parse a sparc object file
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
};
static Addr addr(Biobuf*);
static char type2char(int);
static	void	skip(Biobuf*, int);


int
_isk(char *s)
{
	return  s[0] == ANAME				/* ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}


int
_readk(Biobuf *bp, Prog *p)
{
	int i, as, c;
	Addr a;

	as = Bgetc(bp);			/* as */
	if(as < 0)
		return 0;
	p->kind = aNone;
	if(as == ANAME){
		p->kind = aName;
		p->type = type2char(Bgetc(bp));		/* type */
		p->sym = Bgetc(bp);			/* sym */
		c = Bgetc(bp);
		for(i=0; i < NNAME && c > 0; i++){
			p->id[i] = c;
			c = Bgetc(bp);
		}
		if(i < NNAME)
			p->id[i] = c;
		return 1;
	}
	if(as == ATEXT)
		p->kind = aText;
	else if(as == AGLOBL)
		p->kind = aData;
	skip(bp, 5);		/* reg (1 byte); lineno (4 bytes) */
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
	skip(bp, 1);		/* reg */
	a.sym = Bgetc(bp);	/* sym index */
	a.name = Bgetc(bp);	/* sym type */
	switch(a.type) {
	default:
	case D_NONE: case D_REG: case D_FREG: case D_CREG: case D_PREG:
		break;
	case D_BRANCH:
	case D_OREG:
	case D_ASI:
	case D_CONST:
		off = Bgetc(bp);
		off |= Bgetc(bp) << 8;
		off |= Bgetc(bp) << 16;
		off |= Bgetc(bp) << 24;
		if(off < 0)
			off = -off;
		if(a.sym!=0 && (a.name==D_PARAM || a.name==D_AUTO))
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
