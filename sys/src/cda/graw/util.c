#include <u.h>
#include <libc.h>
#include <libg.h>
#include "thing.h"

extern Bitmap screen,*stage;
extern Mouse mouse;
extern Font *defont;
extern Point grid;

Bitmap *
maptex(Texture *t)
{
	if (t->b == 0) {
		t->b = balloc(Rect(0,0,16,16),0);
		wrbitmap(t->b,0,16,(uchar *)t->a);
	}
	return t->b;
}

Bitmap *
mapblit(Blitmap *b)
{
	if (b->b == 0) {
		b->b = balloc(b->r,b->ldepth);
		wrbitmap(b->b,b->r.min.y,b->r.max.y,(uchar *)b->base);
	}
	return b->b;
}

Rectangle
canon(Rectangle r)
{
	int t;
	if (U < P.x) {
		t = P.x;
		P.x = U;
		U = t;
	}
	if (V < P.y) {
		t = P.y;
		P.y = V;
		V = t;
	}
	return r;
}

dombb(Rectangle *p, Rectangle r)
{
	int t;
	if ((t=r.min.x) < p->min.x)
		p->min.x = t;
	if ((t=r.min.y) < p->min.y)
		p->min.y = t;
	if ((t=r.max.x+1) > p->max.x)	/* +1 because of lines&boxes */
		p->max.x = t;
	if ((t=r.max.y+1) > p->max.y)
		p->max.y = t;
}

Point
origin(void)
{
	Point p;
	p = screen.r.min;
	p.x += GRID - (p.x&GRIDMASK);
	p.y += GRID - (p.y&GRIDMASK);
	return p;
}

Rectangle
mygetrect(int m)
{
	int docursor(int);
	Rectangle r;
	while (!getgrid()) {
		docursor(0);
		filter(minbb);
	}
	r.min = r.max = mouse.xy;
	drawrect(r,&screen,ADD);
	while (getgrid()&m) {
		docursor(0);
		drawrect(r,&screen,SUB);
		r.max = mouse.xy;
		drawrect(r,&screen,ADD);
	}
	drawrect(r,&screen,SUB);
	if (showgrey)
		r = rmul(rsubp(r,origin()),2);
	return canon(r);
}

twostring(Point p,char *a,char *b)
{
	string(&screen,string(&screen,p,defont,a,S^D),defont,b,S^D);	/* show off */
}

alnum(int c)
{
	return ('0'<=c && c<='9') || (c=='_') ||
	       ('a'<=c && c<='z') || ('A'<=c && c<='Z');
}

char *
addachar(char *s,int c)
{
	char *p;
	for (p = s; *p; p++)
		;
	if (c >= ' ')			/* append c */
		*p++ = c;
	else if (c == '\b' && p > s)	/* backspace */
		p--;
	else if (c == 027) {		/* control-W */
		while (p > s && !alnum(*(p-1)))
			p--;
		while (p > s && alnum(*(p-1)))
			p--;
	}
	*p = 0;
}

char *
getline(char *msg,char *buf)
{
	int c;
	Point p;
	p = Pt(screen.r.min.x+10,screen.r.max.y-20);
	twostring(p,msg,buf);
	while ((c = ekbd()) != '\n') {
		twostring(p,msg,buf);
		addachar(buf,c);
		twostring(p,msg,buf);
	}
	twostring(p,msg,buf);
	return buf;
}

complain(char *s0,char *s1)
{
	char buf0[100],buf1[20];
	sprint(buf0,"%s%s (RETURN to continue)",s0,s1);
	getline(buf0,buf1);
}

mtime(int fd)
{
	Dir buf;
	dirfstat(fd,&buf);
	return buf.mtime;
}

Point
gridpt(Point p)
{
	p.x = (p.x + 3) & ~7;
	p.y = (p.y + 3) & ~7;
	return p;
}

Rectangle
gridrect(Rectangle r)
{
	r.min = gridpt(r.min);
	r.max = gridpt(r.max);
	return r;
}

Point
rclipl(Rectangle r,Rectangle s)
{
	Point p,q;
	p = s.min;
	q = s.max;
	if (p.x < r.min.x && q.x >= r.min.x)
		return Pt(r.min.x,p.y);
	if (p.y < r.min.y && q.y >= r.min.y)
		return Pt(p.x,r.min.y);
	if (p.x <= r.max.x && q.x > r.max.x)
		return Pt(r.max.x,p.y);
	if (p.y <= r.max.y && q.y > r.max.y)
		return Pt(p.x,r.max.y);
}

#define NTOK	1021	/* hack, fixed number of tokens */
char *token[NTOK];
int ntok;

strhash(char *s)
{
	int i=0,c;
	while (c = *s++)
		i = (i<<1) ^ c;
	return i&0x7fffffff;
}

char *
intern(char *s)
{
	int i,j;
	char *t;
	for (i = j = strhash(s)%NTOK; t = token[i];) {
		if (strcmp(s,t) == 0) {
			return t;
		}
		if (++i == NTOK)
			i = 0;
		if (i == j)
			print("bart error: out of tokens\n");
	}
	ntok++;
	return token[i] = strcpy(malloc(strlen(s)+1),s);
}

char *
lookup(char *k,Dict *d)
{
	int i;
	char *t;
	if (d->n == 0)
		return 0;
	for (i = (((ulong) k)>>2)%d->n; t = d->key[i];) {
		if (k == t)
			return d->val[i];
		if (++i == d->n)
			i = 0;
	}
	return 0;
}

Dict *
rehash(Dict *d, int n)
{
	int i;
	Dict *t = (Dict *) calloc(1,sizeof(Dict));
	t->n = n;
	t->key = (char **) calloc(1,n*sizeof(char *));
	t->val = (char **) calloc(1,n*sizeof(char *));
	for (i = 0; i < d->n; i++)
		if (d->key[i])
			assign(d->key[i],d->val[i],t);
	free(d->key);
	free(d->val);
	d->n = n;
	d->key = t->key;
	d->val = t->val;
	free(t);
	return d;
}

acmp(char **a,char **b)
{
	return strcmp(*a,*b);
}

alphkeys(Dict *d,char **a)
{
	int i;
	char **p = a;
	for (i = 0; i < d->n; i++)
		if (d->key[i])
			*p++ = d->key[i];
	*p = 0;
	qsort(a,p-a,sizeof(char *),acmp);
}

char *
assign(void *k,void *v,Dict *d)
{
	int i;
	char *t;
	if (d->n == 0) {			/* initialize */
		d->n = 8;
		d->key = (char **) calloc(1,d->n*sizeof(void *));
		d->val = (char **) calloc(1,d->n*sizeof(void *));
	}
	for (i = (((ulong) k)>>2)%d->n; t = d->key[i];) {
		if (k == t)			/* lookup */
			return d->val[i] = v;
		if (++i == d->n)
			i = 0;
	}
	if (++d->used > d->n/2)	{		/* grow */
		rehash(d,2*d->n);
		return assign(k,v,d);
	}
	d->key[i] = k;
	return d->val[i] = v;
}

showkeys(Dict *d)
{
	int i;
	for (i = 0; i < d->n; i++)
		if (d->key[i])
			print("%s ",d->key[i]);
}

dumpdict(void)
{
	int i;
	print("%d tokens:\n",ntok);
	for (i = 0; i < NTOK; i++)
		if (token[i])
			print("%s ",token[i]);
}
