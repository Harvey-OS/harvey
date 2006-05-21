/*
 * 2obj.c - identify and parse a 68020 object file
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "2c/2.out.h"
#include "obj.h"

typedef struct Addr	Addr;
struct Addr
{
	char	sym;
	char	flags;
};
static Addr addr(Biobuf *);
static char type2char(int);
static void skip(Biobuf*, int);

int
_is2(char *t)
{
	uchar *s = (uchar *)t;

	return  s[0] == (ANAME&0xff)			/* aslo = ANAME */
		&& s[1] == ((ANAME>>8)&0xff)		/* ashi = ANAME */
		&& s[2] == D_FILE			/* type */
		&& s[3] == 1				/* sym */
		&& s[4] == '<';				/* name of file */
}

int
_read2(Biobuf *bp, Prog *p)
{
	int as, n, c;
	Addr a;

	as = Bgetc(bp);		/* as(low) */
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
	skip(bp, 4);		/*lineno: low, high, lowhigh, highigh*/
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

	a.flags = Bgetc(bp);			/* flags */
	a.sym = -1;
	off = 0;
	if(a.flags & T_FIELD)
		skip(bp, 2);
	if(a.flags & T_INDEX)
		skip(bp, 7);
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
	else if(a.flags & T_SCONST)
		skip(bp, NSNAME);
	else{
		t = Bgetc(bp);
		if(a.flags & T_TYPE)
			t |= Bgetc(bp) << 8;
		t &= D_MASK;
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
