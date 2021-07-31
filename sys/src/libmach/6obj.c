/*
 * 6obj.c - identify and parse an I960 object file
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "6c/6.out.h"
#include "obj.h"

typedef struct Addr	Addr;
struct Addr
{
	char	sym;
	char	flags;
};
static	Addr	addr(Biobuf*);
static	char	type2char(int);
static	void	skip(Biobuf*, int);

int
_is6(char *t)
{
	uchar *s = (uchar*)t;

	return  s[0] == ANAME				/* aslo = ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}

int
_read6(Biobuf *bp, Prog* p)
{
	int as, i, c;
	Addr a;

	as = Bgetc(bp);		/* as */
	if(as < 0)
		return 0;
	p->kind = aNone;
	if(as == ANAME){
		p->kind = aName;
		p->type = type2char(Bgetc(bp));	/* type */
		p->sym = Bgetc(bp);			/* sym */
		c = Bgetc(bp);
		for(i=0; i<NNAME && c>0; i++){
			p->id[i] = c;
			c = Bgetc(bp);
		}
		if(i < NNAME)
			p->id[i] = c;
		return 1;
	}
	if(as == ATEXT)
		p->kind = aText;
	if(as == AGLOBL)
		p->kind = aData;
	skip(bp, 5);		/* lineno(4 bytes), reg(1 byte)*/
	a = addr(bp);
	addr(bp);
	if(!(a.flags & T_SYM))
		p->kind = aNone;
	p->sym = a.sym;
	return 1;
}

static Addr
addr(Biobuf *bp)
{
	Addr a;
	int t;
	long off;

	off = 0;
	a.sym = -1;
	a.flags = Bgetc(bp);			/* flags */
	if(a.flags & T_INDEX)
		skip(bp, 2);
	if(a.flags & T_OFFSET){
		off = Bgetc(bp);
		off |= Bgetc(bp) << 8;
		off |= Bgetc(bp) << 16;
		off |= Bgetc(bp) << 24;
		if(off < 0)
			off = -off;
	}
	if(a.flags & T_SYM)
		a.sym = Bgetc(bp);
	if(a.flags & T_FCONST)
		skip(bp, 8);
	else
	if(a.flags & T_SCONST)
		skip(bp, NSNAME);
	if(a.flags & T_TYPE) {
		t = Bgetc(bp);
		if(a.sym > 0 && (t==D_PARAM || t==D_AUTO))
			_offset(a.sym, off);
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
