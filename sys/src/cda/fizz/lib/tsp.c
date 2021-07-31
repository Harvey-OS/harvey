#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void rev(short *res, int a, int b, int n);
void reorder(Coord *cb, Pin *pb, short *o, int n);
void lopt2(int n, short *result, Pin *p, int head);
void bopt2(int n, short *result, Pin *p, int head);
long tsp1(Coord *c, Pin *p, short *o, int n, int head);
void ttsp(Coord *cb, Pin *pb, int n);
long exhaust(Coord *, Pin *, short *, int, int);
static char *dbg;
int oneok;

void
tsp(Signal *s)
{
	if(s->type & VSIG)
		return;
	if(s->n == 1){
		if(!oneok)
			fprint(2, "signal %s has only one point!\n", s->name);
		return;
	}
	ttsp(s->coords, s->pins, s->n);
}

long wsx, wsy, wsr;
static Pin *pbase;
static Coord *cbase;

int
xcmp(short *a, short *b)
{
	register k;

	if(k = pbase[*a].p.x - pbase[*b].p.x)
		return(k);
	return(pbase[*a].p.y - pbase[*b].p.y);
}

int
ycmp(short *a, short *b)
{
	register k;

	if(k = pbase[*a].p.y - pbase[*b].p.y)
		return(k);
	return(pbase[*a].p.x - pbase[*b].p.x);
}

void
sortset(int (*fn)(void*, void*), short *order, Coord *cb, Pin *pb, int n)
{
	register i;

	cbase = cb;
	pbase = pb;
	for(i = 0; i < n; i++) order[i] = i;
	qsort(order, (long) n, (long) sizeof(short), fn);
}

void
ttsp(Coord *cb, Pin *pb, int n)
{
	short o[MAXNET], best[MAXNET];
	long bestl, l;

#define	TRY(fn)	{ l = fn(cb, pb, o, n, -1);\
	if(l < bestl){\
		memcpy((char *)best, (char *)o, n*sizeof(short));\
		bestl = l;\
	} }

	bestl = 9999999L;
	switch(n)
	{
	case 2:
		best[0] = 0; best[1] = 1;
		break;
	case 3:
		/* sort first  to be deterministic */
		sortset(ycmp, o, cb, pb, n);
#define	goo(a,b,c)	o[0]=a;o[1]=b;o[2]=c;TRY(exhaust)
		goo(0,1,2); goo(0,2,1); goo(1,0,2);
#undef	goo
		break;
	case 4:
		/* sort first  to be deterministic */
		sortset(ycmp, o, cb, pb, n);
#define	goo(a,b,c,d)	o[0]=a;o[1]=b;o[2]=c;o[3]=d;TRY(exhaust)
		goo(0,1,2,3);
		goo(0,1,3,2);
		goo(0,2,1,3);
		goo(0,2,3,1);
		goo(0,3,1,2);
		goo(0,3,2,1);
		goo(1,0,2,3);
		goo(1,0,3,2);
		goo(1,2,0,3);
		goo(1,3,0,2);
		goo(2,0,1,3);
		goo(2,1,0,3);
#undef	goo
		break;
	default:
		sortset(xcmp, o, cb, pb, n); TRY(tsp1);
		sortset(ycmp, o, cb, pb, n); TRY(tsp1);
	}
	if(bestl);	/* shut up cyntax */
	reorder(cb, pb, best, n);
}

long
exhaust(Coord *c, Pin *p, short *o, int n, int head)
{
	register long len = 0;
	register short d;
	Point p1, p2;

	p1 = p[*o++].p;
	for(n--; n--; p1 = p2){
		p2 = p[*o++].p;
		d = p1.x-p2.x;
		if(d < 0) d = -d;
		len += d;
		d = p1.y-p2.y;
		if(d < 0) d = -d;
		len += d;
	}
	return(len);
}

long
tsp1(Coord *c, Pin *p, short *o, int n, int head)
{
	short *oo = o;
	register i;
	short newo[MAXNET];

	if(0){
		fprint(1, "%s:", dbg);
		for(i = 0; i < n; i++) fprint(1, " %p", p[o[i]].p);
		fprint(1, "\n");
	}
	if(n == MAXNET){
		fprint(2, "damn: tsp1 needs another pin\n");
		return(0L);
	}
	/* add outlier */
	p[n].p.x = p[n].p.y = MAXCOORD;
	o[n] = n;
	if(n >= 9)
		bopt2(n+1, o, p, head);
	else
		lopt2(n+1, o, p, head);
	/* remove outlier */
	for(i = 0; i <= n; i++, o++)
		if(*o == n) break;
	o = oo;
	memcpy((char *)newo, (char *)(&o[i+1]), sizeof(*o)*(n-i));
	memcpy((char *)(&newo[n-i]), (char *)o, sizeof(*o)*i);
	memcpy((char *)o, (char *)newo, sizeof(*o)*n);
	return(exhaust(c, p, o, n, head));
}

#define	Huge	(MAXCOORD*(long)MAXCOORD)
#define	norm(x)	((x)%n)

void
lopt2(int n, short *result, Pin *p, int head)
{
	register short i, j, pa, pb, pc, pd;
	long ac, bd, ab, cd, k;
	int pole = n-1;

#define	dist(r,a,b)	if((a==pole)||(b==pole))\
	r=((a==head)||(b==head))? 0:Huge; else\
	{ r = p[a].p.x-p[b].p.x; if(r<0) r = -r;\
	k = p[a].p.y-p[b].p.y; r += (k<0)? -k:k;}

loop:
	for(i = 0; i < n; i++){
		pa = result[norm(i)];
		pb = result[norm(i+1)];
		for(j = i+2; j < n+i-2; j++){
			pc = result[norm(j)];
			pd = result[norm(j+1)];
			dist(ac, pa, pc);
			dist(bd, pb, pd);
			dist(ab, pa, pb);
			dist(cd, pc, pd);
			if((ac+bd) < (ab+cd)){
				if(norm(n+j-i-1) < norm(n+i-j-1))
					rev(result, i+1, j, n);
				else
					rev(result, j+1, i, n);
				goto loop;
			}
		}
	}
}
#undef	dist

void
bopt2(int n, short *result, Pin *p, int head)
{
	short i, j, pa, pb, pc, pd;
	long ac, bd, ab, cd, k;
	int pole = n-1;
	short tab[MAXNET*MAXNET];

#define	dist(r,a,b)	if((a==pole)||(b==pole))\
	r=((a==head)||(b==head))? 0:Huge; else\
	if((r = tab[a*n+b])==0){ r = p[a].p.x-p[b].p.x; if(r<0) r = -r;\
	k = p[a].p.y-p[b].p.y; r += (k<0)? -k:k; tab[a*n+b] = r;}

	i = (n*n+1)&~1;		/* give memset a mult of 4 */
	memset((char *)tab, 0, i*sizeof(tab[0]));
loop:
	for(i = 0; i < n; i++){
		pa = result[norm(i)];
		pb = result[norm(i+1)];
		for(j = i+2; j < n+i-2; j++){
			pc = result[norm(j)];
			pd = result[norm(j+1)];
			dist(ac, pa, pc);
			dist(bd, pb, pd);
			dist(ab, pa, pb);
			dist(cd, pc, pd);
			if((ac+bd) < (ab+cd)){
				if(norm(n+j-i-1) < norm(n+i-j-1))
					rev(result, i+1, j, n);
				else
					rev(result, j+1, i, n);
				goto loop;
			}
		}
	}
}

void
rev(short *res, int a, int b, int n)
{
	register short i, j;
	register short t;

	if(a > b) b += n;
	while(a < b){
		i = norm(a++);
		j = norm(b--);
		t = res[i];
		res[i] = res[j];
		res[j] = t;
	}
}

void
reorder(Coord *cb, Pin *pb, short *o, int n)
{
	Coord c[MAXNET];
	Pin p[MAXNET];

	memcpy((char *)c, (char *)cb, n*sizeof(Coord));
	memcpy((char *)p, (char *)pb, n*sizeof(Pin));
	while(n--){
		*cb++ = c[*o];
		*pb++ = p[*o++];
	}
}
