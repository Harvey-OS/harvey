/*
 * zobj.c - identify and parse a hobbit object file
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
static Addr addr(Biobuf*);
static char type2char(int);
static void skip(Biobuf*, int);

int
_isz(char *s)
{
	return  s[0] == ANAME				/* ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}

int
_readz(Biobuf *bp, Prog *p)
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
	skip(bp, 4);		/* lineno (low, high, lowhigh, highigh) */
	a = addr(bp);
	if(a.type != D_STATIC && a.type != D_EXTERN)
		p->kind = aNone;
	p->sym = a.sym;
	a = addr(bp);
	if(a.type != D_CONST)
		p->kind = aNone;
	return 1;
}

static Addr
addr(Biobuf *bp)
{
	Addr a;
	int name;
	long off;

	a.type = Bgetc(bp);	/* type */
	skip(bp, 1);		/* width */
	a.sym = Bgetc(bp);	/* sym */
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
		skip(bp, 4);		/* pitch offsets */
		break;
	case D_AUTO:
	case D_PARAM:
		off = Bgetc(bp);
		off |= Bgetc(bp) << 8;
		off |= Bgetc(bp) << 16;
		off |= Bgetc(bp) << 24;
		if(off < 0)
			off = -off;
		if(a.sym)
			_offset(a.sym, off);
		break;
	case D_SCONST:
		skip(bp, Bgetc(bp));	/* get count and skip that many */
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
