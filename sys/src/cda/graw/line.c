#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <libg.h>
#include "thing.h"
extern Bitmap display,*stage;
extern Font *tiny;
extern Dict *dict;
extern File *this;
#define ONES	0xffffffff

mysegment(Bitmap *b,Point p,Point q,Fcode f)
{
	segment(b,p,q,ONES,f);
	if (showgrey) {
		int dx = q.x-p.x, dy = q.y-p.y;
		if (dx < 0)
			dx = -dx;
		if (dy < 0)
			dy = -dy;
		if (dx > dy)
			segment(b,Pt(p.x,p.y+1),Pt(q.x,q.y+1),ONES,f);
		else
			segment(b,Pt(p.x+1,p.y),Pt(q.x+1,q.y),ONES,f);
	}
}

Line *
newline(Point p,Line *n)
{
	Line *l;
	l = (Line *) malloc(sizeof(Line));
	l->type = LINE;
	l->r = Rpt(p,p);
	l->sel = 12;
	l->mod = LINEHIT;
	l->next = n;
	return l;
}

Line *
newbox(Point p,Line *n)
{
	Box *b;
	b = (Box *) malloc(sizeof(Box));
	b->type = BOX;
	b->r = Rpt(p,p);
	b->sel = 12;
	b->mod = BOXHIT;
	b->next = n;
	return (Line *) b;
}

Line *
newdots(Point p,Line *n)
{
	Box *b;
	b = (Box *) malloc(sizeof(Box));
	b->type = DOTS;
	b->r = Rpt(p,p);
	b->sel = 12;
	b->mod = BOXHIT;
	b->next = n;
	return (Line *) b;
}

Line *
newmacro(Point p,Line *n)
{
	Box *b;
	b = (Macro *) malloc(sizeof(Macro));
	b->type = MACRO;
	b->r = Rpt(p,p);
	b->sel = 12;
	b->mod = BOXHIT;
	b->next = n;
	return (Line *) b;
}

Line *
newstring(Point p,int code,Line *n)
{
	String *t;
	t = (String *) malloc(sizeof(String));
	t->type = STRING;
	t->r = Rpt(p,p);
	t->len = 8;
	t->s = (char *) malloc(t->len);
	*(t->s) = 0;
	t->sel = 3;
	t->mod = STRHIT;
	t->next = n;
	t->code = code;
	return (Line *) t;
}

setstring(String *s,char *t)
{
	int i = strlen(t)+1;
	if (i >= s->len) {
		free(s->s);
		s->len = 8*(i/8)+8;
		s->s = (char *) malloc(s->len);
	}
	strcpy(s->s,t);
}

Line *
newinst(Point p,char *s,Line *n)
{
	Inst *i;
	i = (Inst *) malloc(sizeof(Inst));
	i->type = INST;
	i->r = Rpt(p,p);
	i->sel = 15;
	i->mod = BOXHIT;
	i->next = n;
	i->name = s;
	return (Line *) i;
}

Line *
newref(char *s,Line *n)
{
	Ref *r;
	r = (Ref *) malloc(sizeof(Ref));
	r->type = REF;
	r->next = n;
	r->filename = s;
	return (Line *) r;
}

addaref(Line **dl,char *s)		/* takes a pointer to a dl */
{
	Line *l;
	for (l = *dl; l; l = l->next)
		if (l->type == REF && ((Ref *) l)->filename == s)
			return;
	*dl = newref(s,*dl);
}

Line *
newmaster(char *s,Line *n)
{
	Master *m;
	m = (Master *) calloc(1,sizeof(Master));
	m->type = MASTER;
	m->next = n;
	m->name = s;
	return (Line *) m;
}

selpt(Line *dl,Point p)
{
	int s,m,prop;
	Rectangle r;
	Line *l=dl;
	for (m = 0; l; l = l->next) {
		r = l->r;
		prop = 0;
		switch (l->type) {
		case LINE:
			if (EQ(p,P))
				l->sel = 3;
			else if (EQ(p,Q))
				l->sel = 12;
			else if (Y == p.y && Y == V && (p.x-X)*(U-p.x) > 0)
				prop = 1;
			else if (X == p.x && X == U && (p.y-Y)*(V-p.y) > 0)
				prop = 1;
			else
				continue;
			m |= (l->mod |= LINEHIT);
			if (prop) {
				l->sel = 15;
				m |= selbox(dl,canon(r));
			}
			break;
		case BOX:
		case MACRO:
			if (EQ(p,ul(r)))
				selborder(dl,l->sel=3,r);
			else if (EQ(p,lr(r)))
				selborder(dl,l->sel=12,r);
			else if (EQ(p,ll(r)))
				selborder(dl,l->sel=9,r);
			else if (EQ(p,ur(r)))
				selborder(dl,l->sel=6,r);
			else if (l->type == BOX && IN(p,r)) {
				l->sel = 15;
				m |= selbox(dl,r);
			}
			else
				continue;
			m |= (l->mod |= BOXHIT);
			break;
		case INST:
			if (WITHIN(p,r)) {
				l->sel = 15;
				m |= (l->mod |= BOXHIT) | selbox(dl,r);
			}
			break;	
		case DOTS:
			if (EQ(p,ul(r)))
				l->sel=3;
			else if (EQ(p,lr(r)))
				l->sel=12;
			else if (EQ(p,ll(r)))
				l->sel=9;
			else if (EQ(p,ur(r)))
				l->sel=6;
			else if (IN(p,r))
				l->sel = 15;
			else
				continue;
			m |= (l->mod |= DOTHIT);
			break;
		case STRING:
			if (EQ(p,P)) {
				l->sel = 3;
				m |= (l->mod |= STRHIT);
			}
			break;
		}
	}
	return m;
}

selbox(Line *l,Rectangle r)
{
	int m;
	for (m = 0; l; l = l->next)
		switch(l->type) {
		case LINE:
			if (WITHIN(l->P,r)) {
				l->sel |= 3;
				m |= (l->mod |= LINEBOX);
			}
			if (WITHIN(l->Q,r)) {
				l->sel |= 12;
				m |= (l->mod |= LINEBOX);
			}
			break;
		case BOX:
		case MACRO:
		case DOTS:
		case INST:
			if (WITHIN(l->P,r) && WITHIN(l->Q,r)) {
				l->sel |= 15;
				m |= (l->mod |= BOXBOX);
			}
			break;
		case STRING:
			if (WITHIN(l->P,r)) {
				l->sel = 3;
				m |= (l->mod |= STRBOX);
			}
			break;
		}
	return m;
}

slashbox(Line *l,Rectangle r)
{
	int m;
	for (m = 0; l; l = l->next)
		switch(l->type) {
		case LINE:
			if (WITHIN(l->P,r)) {
				l->sel |= 3;
				m |= (l->mod |= LINEBOX);
			}
			if (WITHIN(l->Q,r)) {
				l->sel |= 12;
				m |= (l->mod |= LINEBOX);
			}
			if (l->sel != 0 && l->sel != 15
				&& (l->X == l->U || l->Y == l->V)) {	/* clip */
				Line *n = newline(l->Q,l->next);
				l->next = n;
				l->Q = n->P = rclipl(r,canon(l->r));
				n->sel = l->sel&12;
				n->sel |= n->sel>>2;
				l->sel &= 3;
				l->sel |= l->sel<<2;
				l = l->next;
			}
			break;
		case BOX:
		case MACRO:
		case DOTS:
		case INST:
			if (WITHIN(l->P,r) && WITHIN(l->Q,r)) {
				l->sel |= 15;
				m |= (l->mod |= BOXBOX);
			}
			break;
		case STRING:
			if (WITHIN(l->P,r)) {
				l->sel = 3;
				m |= (l->mod |= STRBOX);
			}
			break;
		}
	return m;
}

edge(int mask,Point p,Rectangle r)
{
	if (!(WITHIN(p,r)))
		return 0;
	if ((mask&1) && p.x == X || (mask&4) && p.x == U)
		return 1;
	if ((mask&2) && p.y == Y || (mask&8) && p.y == V)
		return 2;
	return 0;
}

selborder(Line *l,int mask,Rectangle r)
{
	int m;
	for (m = 0; l; l = l->next)
		switch(l->type) {
		case LINE:
			if (l->sel |= (edge(mask,l->Q,r)<<2))
				m |= (l->mod |= LINEBOX);
			if (l->sel |= edge(mask,l->P,r))
				m |= (l->mod |= LINEBOX);
			break;
		case STRING:
			if (l->sel |= edge(mask,l->P,r))
				m |= (l->mod |= STRBOX);
			break;
		}
	return m;
}

textcode(Line *l,Point p)
{
	int c=0,lc=0,bc=0,sc=0;
	for (; l; l = l->next, c = 0)
		switch(l->type) {
		case LINE:
			if (l->X == l->U) {		/* vertical */
				if (EQ(p,l->P))
					c = (l->Y < l->V) ? HALFX|FULLY : HALFX;
				else if (EQ(p,l->Q))
					c = (l->V < l->Y) ? HALFX|FULLY : HALFX;
			}
			else if (l->Y == l->V) {	/* horizontal */
				if (EQ(p,l->P) && (l->X > l->U) || EQ(p,l->Q) && (l->U > l->X))
					c = FULLX|FULLY;
			}
			else
				continue;
			lc |= c;
			break;
		case MACRO:
			if (p.x < l->X || p.y < l->Y || p.x > l->U || p.y > l->V)
				continue;
			if (p.x == l->X)
				c = FULLX | HALFY;
			else if (p.x == l->U)
				c = HALFY;
			else if (p.y == l->Y)
				c = HALFX | FULLY;
			else if (p.y == l->V)
				c = HALFX;
			bc |= c;
			break;
		case BOX:
		case INST:
			if (p.x < l->X || p.y < l->Y || p.x > l->U || p.y > l->V)
				continue;
			if (p.x == l->X)
				c = HALFY;
			else if (p.x == l->U)
				c = FULLX | HALFY;
			else if (p.y == l->Y)
				c = HALFX;
			else
				c = HALFX | FULLY;
			bc |= c;
			break;
		}
	if ((lc&(FULLX|HALFX)) == (FULLX|HALFX))	/* vert is wimpy */
		lc &= ~HALFX;
	else if (lc == 0)
		lc = FULLY;
	return bc ? bc : lc;
}

losembb(Line *l)
{
	for (; l; l = l->next)
		if (l->type == MASTER)
			bfree(((Master *)l)->b);
}

Rectangle
mastermbb(Master *m,Dict *d)
{
	Rectangle r;
	if (m->b == 0) {
		r = mbb(m->dl,d);
		Q = add(Q,Pt(2,2));
		m->b = balloc(r,stage->ldepth);
		draw(m->dl,m->b,SADD,1);
	}
	r = m->b->r;
	Q = sub(Q,Pt(2,2));
	return r;
}

Rectangle
mbb(Line *l,Dict *d)
{
	int i;
	Point p,q;
	Rectangle r,rr;
	Master *m;
	r = MAXBB;
	for (; l; l = l->next)
		switch (l->type) {
		case LINE:		/* not necessarily ordered P & Q */
			X = lo(X,l->U);
			Y = lo(Y,l->V);
			U = hi(U,l->X);
			V = hi(V,l->Y);
		case BOX:
		case MACRO:
		case DOTS:
			X = lo(X,l->X);
			Y = lo(Y,l->Y);
			U = hi(U,l->U);
			V = hi(V,l->V);
			break;
		case STRING:
			if (((String *)l)->code & INVIS)
				break;
			p = l->P;
			q = strsize(tiny,((String *)l)->s);
			if ((i = ((String *)l)->code) & FULLX)
				p.x -= q.x;
			else if (i&HALFX)
				p.x -= q.x/2;
			if (i&FULLY)
				p.y -= q.x;
			else if (i&HALFY)
				p.y -= q.y/2;
			q = add(p,q);
			X = lo(X,p.x);
			Y = lo(Y,p.y);
			U = hi(U,q.x);
			V = hi(V,q.y);
			break;
		case INST:
			if (m = (Master *) lookup(((Inst *)l)->name,d)) {
				rr = raddp(mastermbb(m,d),l->P);
				l->r = rr;		/* assumes 0,0 ul corner */
				X = lo(X,rr.min.x);	/* tried rX here, ken didn't buy it */
				Y = lo(Y,rr.min.y);
				U = hi(U,rr.max.x);
				V = hi(V,rr.max.y);
			}
			else
				print("couldn't find %s\n",((Inst *)l)->name);
			break;
		case MASTER:
			mastermbb((Master *)l,d);
			break;
		}
	return r;
}

addchars(String *l,char *s)
{
	char *n;
	drawstring((String *)l,stage,SUB);
	if (strlen(s) + strlen(l->s) + 1 > l->len) {
		n = (char *) malloc(l->len <<= 1);
		strcpy(n,l->s);
		free(l->s);
		l->s = n;
	}
	while (*s)
		addachar(l->s,*s++);
	drawstring((String *)l,stage,ADD);
}

int boxy[] = {0, 1, 1, 1, 0, 0, 0, 0, 0};	/* needs to be canonized */

/*
 * magic stuff ahead
 *	when walking through a list with potential deletions, we need to
 *	treat dl as a list (to smash its ->next) and we need to be careful
 *	not to retain pointers to deleted items
 */

unselect(Line **dl,int mask)
{
	Line *k=(Line *)dl,*l;
	for (l = k->next; l; l = l->next) {
		if (boxy[l->type]) {
			if (l->sel != 15)
				l->r = canon(l->r);
			if (l->X == l->U || l->Y == l->V) {
				drawrect(l->r,stage,ADD);
				k->next = l->next;
				free(l);
				l = k;
				continue;
			}
		}
		else if (l->type == LINE && EQ(l->P,l->Q)) {
			k->next = l->next;
			free(l);
			l = k;
			continue;
		}
		if ((l->mod &= ~mask) == 0)
			l->sel = 0;
		k = l;
	}
}

snap(Line **dl)
{
	Line *k=(Line *)dl,*l;
	String *s;
	int code;
	minbb = MAXBB;
	for (l = k->next; l; l = l->next) {
		if (l->sel) switch(l->type) {
		case LINE:
			if ((l->X == l->U || l->Y == l->V)
				&& collinear(*dl,l,canon(l->r))) {
				mysegment(stage,l->P,l->Q,ADD);
				k->next = l->next;
				free(l);
				l = k;
				continue;
			}
			break;
		case STRING:
			s = (String *)l;
			if ((code=textcode(*dl,s->P)) != s->code) {
				drawstring(s,stage,ADD);
				s->code = code;
				drawstring(s,stage,ADD);
			}
			break;
		}
		k = l;
	}
	filter(minbb);
}

chirp(char *s)
{
	print(s);
	return 1;
}

#define TWEEN(x,a,b)	(a <= x && x <= b)

collinear(Line *l,Line *me,Rectangle r)
{
	int i,j;
	if (Y == V) {
		for (; l; l = l->next)
			if (l->type == LINE && l != me
				&& l->Y == Y && l->V == Y
				&& (TWEEN(l->X,X,U)
					|| TWEEN(l->U,X,U))) {
				X = lo(X,lo(l->X,l->U));
				U = hi(U,hi(l->X,l->U));
				goto doit;
			}
		return 0;
	}
	else {
		for (; l; l = l->next)
			if (l->type == LINE && l != me
				&& l->X == X && l->U == X
				&& (TWEEN(l->Y,Y,V)
					|| TWEEN(l->V,Y,V))) {
				Y = lo(Y,lo(l->Y,l->V));
				V = hi(V,hi(l->Y,l->V));
				goto doit;
			}
		return 0;
	}
doit:
	mysegment(stage,l->P,l->Q,ADD);
	l->r = r;
	mysegment(stage,l->P,l->Q,ADD);
	return 1;
}

move(Line *l,Point dp)
{
	int sel;
	for (; l; l = l->next) {
		if ((sel = l->sel) == 0)
			continue;
		if (sel&1)
			l->X += dp.x;
		if (sel&2)
			l->Y += dp.y;
		if (sel&4)
			l->U += dp.x;
		if (sel&8)
			l->V += dp.y;
	}
}

allmove(Line *l,Point dp)
{
	int sel;
	for (; l; l = l->next) {
		l->X += dp.x;
		l->Y += dp.y;
		l->U += dp.x;
		l->V += dp.y;
	}
}

drawrect(Rectangle r,Bitmap *b,Fcode f)
{
	mysegment(b,ul(r),ur(r),f);
	mysegment(b,ur(r),lr(r),f);
	mysegment(b,lr(r),ll(r),f);
	mysegment(b,ll(r),ul(r),f);
	dombb(&minbb,r);
}

drawstring(String *s,Bitmap *b,Fcode f)
{
	int code=s->code;
	Point p,d;
	if (code & INVIS)
		return;
	p = s->P;
	d = strsize(tiny,s->s);
	if (code&HALFX)
		p.x -= d.x/2;
	else if (code&FULLX)
		p.x -= d.x+1;
	if (code&HALFY)
		p.y -= d.y/2;
	else if (code&FULLY)
		p.y -= d.y;
	p.x++;
	string(b,p,tiny,s->s,f);
	point(b,s->P,1,f);
	p.x--;		/* to get the dot - gombb picks up slack in max */
	dombb(&minbb,Rpt(p,add(p,strsize(tiny,s->s))));
}

drawinst(Inst *i,Bitmap *b,Fcode f)
{
	Master *m;
	if (m = (Master *) lookup(i->name,this->d)) {
		bitblt(b,i->P,m->b,m->b->r,f);
		dombb(&minbb,raddp(m->b->r,i->P));
	}
}

Texture grey = {
	0xaaaa, 0x5555, 0xaaaa, 0x5555,
	0xaaaa, 0x5555, 0xaaaa, 0x5555,
	0xaaaa, 0x5555, 0xaaaa, 0x5555,
	0xaaaa, 0x5555, 0xaaaa, 0x5555,
};

drawmacro(Rectangle r,Bitmap *b,Fcode f)
{
/*	texture(b,r,maptex(&grey),f);
	texture(b,inset(r,1),maptex(&grey),f);
 */
	int i=1;
	if (b->ldepth > 1)
		i = (1 << ((1 << b->ldepth) - 1)) -1;
	segment(b,r.min,ur(r),i,f);
	segment(b,ur(r),r.max,i,f);
	segment(b,r.max,ll(r),i,f);
	segment(b,ll(r),r.min,i,f);
	dombb(&minbb,r);
}

Texture tdots = {
	0xe0e0, 0xe0e0, 0xe0e0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xe0e0 ,0xe0e0, 0xe0e0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

drawdots(Rectangle r,Bitmap *b,Fcode f)
{
	texture(b,r,maptex(&tdots),f);
	dombb(&minbb,r);
}

draw(Line *l,Bitmap *b,Fcode f,int all)
{
	for (; l; l = l->next)
		if (all || l->sel)
			switch (l->type) {
			case LINE:
				mysegment(b,l->P,l->Q,f);
				dombb(&minbb,canon(l->r));
				break;
			case BOX:
				drawrect(l->r,b,f);
				break;
			case DOTS:
				drawdots(canon(l->r),b,f);
				break;
			case STRING:
				drawstring((String *)l,b,f);
				break;
			case INST:
				drawinst((Inst *)l,b,f);
				break;
			case MACRO:
				drawmacro(canon(l->r),b,f);
				break;
			}
}

Line *
cut(Line **dl)
{
	Line *l=(Line *)dl,*m,*n=0;
	for (m = l, l = l->next; l;)
		if (l->sel) {			/* delete it */
			m->next = l->next;
			l->next = n;
			n = l;
			l = m->next;
		}
		else {				/* keep it */
			m = l;
			l = l->next;
		}
	return n;
}

removel(Line *l)
{
	Line *m;
	for (; l; l = m) {
		m = l->next;
		free(l);
	}
}

append(Line **dl,Line *m)
{
	Line *l=(Line *)dl;
	for (; l->next; l = l->next)
		;
	l->next = m;
}

Line *
copy(Line *l)
{
	Line *m=0;
	for (; l; l = l->next)
		if (l->sel) {
			switch(l->type) {
			case LINE:
				m = newline(l->P,m);
				m->Q = l->Q;
				m->sel = 15;
				break;
			case BOX:
				m = newbox(l->P,m);
				m->Q = l->Q;
				m->sel = 15;
				break;
			case DOTS:
				m = newdots(l->P,m);
				m->Q = l->Q;
				m->sel = 15;
				break;
			case STRING:
				m = newstring(l->P,((String *)l)->code,m);
				setstring((String *)m,((String *)l)->s);
				break;
			case INST:
				m = newinst(l->P,((Inst *)l)->name,m);
				m->Q = l->Q;
				m->sel = 15;
				break;
			}
		}
	return m;
}

putline(FILE *fp,Line *l,int nonm)
{
	int i = (showgrey) ? -1 : 0;
	Point dp;
	String *s;
	dp = sub(Pt(0,0),and(Pt(~GRIDMASK<<showgrey,~GRIDMASK<<showgrey),mbb(l,this->d).min));
	for (; l; l = l->next)
		if (nonm) switch(l->type) {
		case LINE:
			fprintf(fp,"l %d %d %d %d\n",rshift(raddp(l->r,dp),i));
			break;
		case BOX:
			fprintf(fp,"b %d %d %d %d\n",rshift(raddp(l->r,dp),i));
			break;
		case DOTS:
			fprintf(fp,"d %d %d %d %d\n",rshift(raddp(l->r,dp),i));
			break;
		case MACRO:
			fprintf(fp,"z %d %d %d %d\n",rshift(raddp(l->r,dp),i));
			break;
		case STRING:
			s = (String *)l;
			fprintf(fp,"s \"%s\" %d %d %d\n",s->s,s->code,rshift(raddp(s->r,dp),i).min);
			break;
		case INST:
			fprintf(fp,"i \"%s\" %d %d\n",((Inst *)l)->name,rshift(raddp(l->r,dp),i).min);
			break;
		}
		else switch(l->type) {
		case REF:
			fprintf(fp,"r \"%s\"\n",((Ref *)l)->filename);
			break;
		case MASTER:
			fprintf(fp,"m \"%s\"\n",((Master *)l)->name);
			putline(fp,((Master *)l)->dl,1);
			fprintf(fp,"e\n");
			break;
		}
}
