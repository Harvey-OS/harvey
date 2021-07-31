/*
 * 6 - print symbols in a .6 file
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
static	Addr	addr(void);
static	char	type2char(int);

int
_is6(char *t)
{
	uchar *s = (uchar*)t;

	return  s[0] == ANAME				/* aslo = ANAME */
		&& s[1] == D_FILE			/* type */
		&& s[2] == 1				/* sym */
		&& s[3] == '<';				/* name of file */
}

Prog*
_read6(Prog* p)
{
	int as, i, c;
	Addr a;

	as = Bgetc(_bin);		/* as */
	if(as < 0)
		return 0;
	p->kind = aNone;
	if(as == ANAME){
		p->kind = aName;
		p->type = type2char(Bgetc(_bin));	/* type */
		p->sym = Bgetc(_bin);			/* sym */
		c = Bgetc(_bin);
		for(i=0; i<NNAME && c>0; i++){
			p->id[i] = c;
			c = Bgetc(_bin);
		}
		if(i < NNAME)
			p->id[i] = c;
		return p;
	}
	if(as == ATEXT)
		p->kind = aText;
	if(as == AGLOBL)
		p->kind = aData;
	Bgetc(_bin);		/* lineno(4) */
	Bgetc(_bin);
	Bgetc(_bin);
	Bgetc(_bin);
	Bgetc(_bin);		/* reg */
	a = addr();
	addr();
	if(!(a.flags & T_SYM))
		p->kind = aNone;
	p->sym = a.sym;
	return p;
}

static Addr
addr(void)
{
	Addr a;
	int t, i;
	long off;

	t = 0;
	a.flags = Bgetc(_bin);			/* flags */
	a.sym = -1;
	if(a.flags & T_INDEX)
		for(i=0; i<2; i++)
			Bgetc(_bin);
	off = 0;
	if(a.flags & T_OFFSET){
		off = Bgetc(_bin);
		off |= Bgetc(_bin) << 8;
		off |= Bgetc(_bin) << 16;
		off |= Bgetc(_bin) << 24;
		if(off < 0)
			off = -off;
	}
	if(a.flags & T_SYM)
		a.sym = Bgetc(_bin);
	if(a.flags & T_FCONST)
		for(i=0; i<8; i++)
			Bgetc(_bin);
	else
	if(a.flags & T_SCONST)
		for(i=0; i<NSNAME; i++)
			Bgetc(_bin);
	if(a.flags & T_TYPE)
		t = Bgetc(_bin);
	if(a.sym > 0 && (t==D_PARAM || t==D_AUTO))
		_offset(a.sym, type2char(t), off);
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
