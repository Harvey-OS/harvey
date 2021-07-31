#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

int	lineno;
char *	infile;

Clump *	allclump;

static Clump *	curclump;

Sym *
xyread(char *name)
{
	Biobuf in;
	int infd;
	char *l;

	if(name){
		infile = name;
		infd = open(infile, OREAD);
		if(infd < 0){
			perror(infile);
			exits("open");
		}
	}else{
		infd = 0;
		infile = "/fd/0";
	}
	Binit(&in, infd, OREAD);
	if(!allclump)
		allclump = new(CLUMP);
	lineno = 0;
	while(l = Brdline(&in, '\n')){	/* assign = */
		++lineno;
		l[BLINELEN(&in)-1] = 0;
		if(*l == '*')
			continue;
		if(!curclump)
			getclump(l);
		else
			fillclump(l);
	}
	if(debug)
		Bprint(&out, "%s: %d lines\n", infile, lineno);
	if(debug > 3)
		symprint(&out);
	Bflush(&out);

	return allclump->n ? allclump->o[allclump->n-1] : 0;
}

void
getclump(char *l)
{
	char *p, *q;
	Sym *s;

	p = nextstr(&l);
	q = nextstr(&l);
	if(strcmp(q, "CLUMP") != 0)
		error("expected CLUMP");
	s = symstore(p);
	if(s->type)
		error("redefinition of %s CLUMP", l);
	s->type = Clname;
	if(debug > 2)
		Bprint(&out, "%s %d: define %s CLUMP\n",
			infile, lineno, s->name);
	s->ptr = new(CLUMP);
	curclump = s->ptr;
	wrclump(allclump, s);
}

void
fillclump(char *l)
{
	char *p, *q;
	Sym *s, *t;
	Point pt, corner[3];
	Clump *c;
	Inc *ip; Path *pp; Rect *rp; Repeat *np; Text *tp;
	Ring *kp;
	int scale;

	c = curclump;
	p = nextstr(&l);
	s = symfind(p);
	if(s == 0 || s->type != Keyword)
		error("unknown keyword %s", p);
	switch((int)s->ptr){
	case END:
		p = nextstr(&l);
		if(strcmp(p, "CLUMP") != 0)
			error("botched END");
		endclump(c);
		curclump = 0;
		if(debug > 2){
			s = allclump->o[allclump->n-1];
			Bprint(&out, "%s %d: end %s CLUMP\n",
				infile, lineno, s->name);
		}
		break;
	case INC:
		ip = new(INC);
		s = symfind(p = nextstr(&l));
		if(s == 0 || s->type != Clname)
			error("unknown CLUMP %s", p);
		ip->clump = s;
		ip->pt = nextpt(&l);
		scale = nextint(&l, 1);
		ip->angle = nextint(&l, 0);
		wrclump(c, ip);
		break;
	case REPEAT:
		np = new(REPEAT);
		s = symfind(p = nextstr(&l));
		if(s == 0 || s->type != Clname)
			error("unknown CLUMP %s", p);
		np->clump = s;
		np->pt = nextpt(&l);
		np->count = nextint(&l, 1);
		np->inc.x = nextint(&l, 0);
		np->inc.y = nextint(&l, 0);
		wrclump(c, np);
		break;
	case PATH:
	case BLOB:
		pp = new((int)s->ptr);
		pp->layer = getlayer(nextstr(&l));
		if(pp->type == PATH)
			pp->width = nextint(&l, Required);
		else
			pp->width = 0;
		while(*l)
			wrpath(pp, nextpt(&l));
		endpath(pp);
		wrclump(c, pp);
		break;
	case ETC:
		if(c->n <= 0)
			error("ETC out of order");
		pp = c->o[c->n-1];
		if(pp->type != PATH && pp->type != BLOB)
			error("ETC out of order");
		while(*l)
			wrpath(pp, nextpt(&l));
		endpath(pp);
		break;
	case RING:
		kp = new(RING);
		kp->layer = getlayer(nextstr(&l));
		kp->pt = nextpt(&l);
		kp->diam = nextint(&l, Required);
		wrclump(c, kp);
		break;
	case RECT:
	case RECTI:
		rp = new((int)s->ptr);
		rp->layer = getlayer(nextstr(&l));
		rp->min = nextpt(&l);
		rp->max = nextpt(&l);
		if((int)s->ptr == RECTI)
			rp->max = add(rp->min, rp->max);
		wrclump(c, rp);
		break;
	case TEXT:
		tp = new(TEXT);
		tp->layer = getlayer(nextstr(&l));
		tp->size = nextint(&l, Required);
		tp->start = nextpt(&l);
		scale = nextint(&l, 0);
		tp->angle = nextint(&l, 0);
		p = nextstr(&l);
		if(*p != '(')
			error("bad TEXT string");
		q = strchr(++p, ')');
		if(q == 0)
			error("bad TEXT string");
		*q = 0;
		tp->text = symstore(p);
		wrclump(c, tp);
		break;
	}
}

Sym *
getlayer(char *p)
{
	Sym *s;

	s = symstore(p);
	if(s->type == 0){
		s->type = Layer;
		if(debug > 2)
			Bprint(&out, "%s %d: ref layer %s\n",
				infile, lineno, s->name);
	}else if(s->type != Layer)
		error("bad layer \"%s\"", p);
	return s;
}
