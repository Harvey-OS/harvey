#include <u.h>
#include <libc.h>
#include "sphere.h"
#include "obj.h"

double	MAXD1	= 0.0;
double	MAXD2	= 0.0;
double	LATD1, LNGD1;
#define	NNULL	((Node*)0)

typedef
struct	node
{
	Obj	obj;
	struct	node	*p;
	float	d;
} Node;
Node	node[1500];
Obj	*sp, *dp;
int	nnode;
int	verb;

extern	int	cmp(void *a1, void *a2);
extern	void	printto(Node*);
extern	Obj*	look(char*);
extern	void	xload(char *a1, char *a2);
extern	int	isvor(Obj *p);

void
main(int argc, char *argv[])
{
	Obj *p, *q;
	Node *np, *bp;
	double bd, d, td, pd;
	int i, j;

	q = 0;
	fmtinstall('O', Ofmt);
	while(argc > 1 && argv[1][0] == '-') {
		d = atof(&argv[1][1]);
		if(MAXD1 == 0.0)
			MAXD1 = d; else
		if(MAXD2 == 0.0)
			MAXD2 = d; else
			print("3rd fp arg ignored\n");
		argc--;
		argv++;
	}
	if(MAXD1 == 0.0)
		MAXD1 = 100.0;
	if(MAXD2 == 0.0)
		MAXD2 = MAXD1/2.0;
	LATD1 = MAXD1 / 3444.;
	LNGD1 = MAXD1 / 1105.;
	if(argc < 3) {
		print("plan [-adist [-vdist]] from to [verb]\n");
		exits("argc");
	}
	verb = argc > 3;
	xload(argv[1], argv[2]);
	sp = look(argv[1]);
	if(sp == ONULL) {
		print("cannot find %s\n", argv[1]);
		exits("look");
	}
	dp = look(argv[2]);
	if(dp == ONULL) {
		print("cannot find %s\n", argv[2]);
		exits("look");
	}
	td = dist(sp, dp);
	if(verb)
	print("total distance = %.2f\n", td);
	j = 0;
	for(i=0; i<nnode; i++) {
		np = node+i;
		np->p = NNULL;
		np->d = dist(sp, &np->obj);
/*
		d = dist(dp, &np->obj);
		if(d+np->d > 1.2*td) {
			np->d = 1.0e6;
			j++;
		}
 */
	}
	if(verb)
	print("%d objects rejected\n", j);
	qsort(node, nnode, sizeof(node[0]), cmp);
	sp = look(argv[1]);
	dp = look(argv[2]);
	for(i=1; i<nnode; i++) {
		p = &node[i].obj;
		pd = node[i].d;
		if(verb)
		print("%s %.2f: ", p->name, pd);
		bd = 1e6;
		bp = NNULL;
		for(j=0; j<i; j++) {
			d = pd - node[j].d;
			if(d > MAXD1)
				continue;
			q = &node[j].obj;
			d = p->lat - q->lat;
			if(d < 0)
				d = -d;
			if(d > LATD1)
				continue;
			d = p->lng - q->lng;
			if(d < 0)
				d = -d;
			if(d > LNGD1)
				continue;
			d = dist(p, q);
			if(d > MAXD1)
				continue;
			if(d > MAXD2)
				if((!isvor(p) && p == dp) ||
				   (!isvor(q) && q == sp))
					continue;
			d += node[j].d;
			if(d < bd) {
				bd = d;
				bp = node+j;
			}
		}
		node[i].p = bp;
		node[i].d = bd;
		if(bp == NNULL) {
			if(verb)
			print("cannot reach\n");
			continue;
		}
		if(verb)
		print("to %s %.2f %.4f\n", bp->obj.name,
			bd, dist(q, sp)/bd);
		if(p == dp)
			break;
	}
	print("dist");
	printto(&node[i]);
	print("\n");
	exits(0);
}

int
cmp(void *a1, void *a2)
{
	Node *p1, *p2;

	p1 = a1;
	p2 = a2;
	if(p1->d > p2->d)
		return 1;
	if(p1->d < p2->d)
		return -1;
	return 0;
}

void
printto(Node *n)
{

	if(n != NNULL) {
		printto(n->p);
		print(" %O", &n->obj);
	}
}

int
isvor(Obj *p)
{

	return p->type == 1 || p->type == 7;
}

Obj*
look(char *s)
{
	int i;

	for(i=0; i<nnode; i++)
		if(ocmp(s, &node[i].obj))
			return &node[i].obj;
	return ONULL;
}

void
xload(char *a1, char *a2)
{
	File o[300], *fo;
	Obj oj;
	int c, f, j;

	f = open(oname, 0);
	if(f < 0) {
		print("cannot open %s\n", oname);
		exits("open");
	}

	if(mkhere(o)) {
		c = 1;
		goto srch;
	}

loop:
	c = read(f, o, sizeof(o));
	c /= sizeof(o[0]);
	if(c <= 0) {
		close(f);
		return;
	}

srch:
	for(j=0; j<c; j++) {
		fo = o+j;
		memcpy(oj.name, fo->name, sizeof(oj.name));
		oj.type = fo->type;
		oj.flag = fo->flag;
		oj.lat = atof(fo->lat);
		oj.lng = atof(fo->lng);
		oj.freq = atof(fo->freq);
		if(isvor(&oj) || ocmp(a1, &oj) || ocmp(a2, &oj)) {
			node[nnode].obj = oj;
			node[nnode].p = NNULL;
			node[nnode].d = 0.;
			nnode++;
		}
	}
	goto loop;
}
