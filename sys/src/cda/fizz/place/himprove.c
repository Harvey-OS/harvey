#include <u.h>
#include <libc.h>
#include	<stdio.h>
#include	<cda/fizz.h>
#include	"host.h"
#include	"proto.h"

extern long wwraplen;
Chip *swapa, *swapb;
long bestwraplen;
static int donewrap = 0;
static int gothup;
extern char mydir[];

void sigenable(void);
void rewrap(Chip *);
void sigdisable(void);
int chipswap(Chip *, Chip *);
void improv1(Chip *, Chip *);

void
restartimp(void)
{
	donewrap = 0;
}

void
improve(int *cn, int ncn, int loop)
{
	Chip *todo[1000];
	register i, j;
 	char buf[100];

 	sprint(buf, "%s/fizz.save", mydir);

	if(!donewrap){
		donewrap = 1;
		if (dbfp) fprintf(dbfp, "doing whole wrap:");
		wwraplen = 0;
		wwires = 0;
		routedefault(RTSP);
		symtraverse(S_SIGNAL, wrap);
	}
	gothup = 0;
	put1(IMPROVES);
	putl(-wwraplen);
	if (dbfp) fprintf(dbfp, "improve %d chips: wwraplen=%d\n", ncn, wwraplen);
	for(i = 0; i < ncn; i++)
		todo[i] = cofn(cn[i]);
	sigenable();
	while((loop-- > 0) && !gothup){
		bestwraplen = wwraplen;
		swapa = swapb = 0;
		for(i = 1; i < ncn; i++)
			for(j = 0; j < i; j++){
				improv1(todo[i], todo[j]);
				if(gothup) goto done;
			}
	done:
		if(swapa){
			if (dbfp) fprintf(dbfp, "swap[%s<->%s], improved by %d to %d\n",
				swapa->name, swapb->name, wwraplen-bestwraplen, bestwraplen);
			chipswap(swapa, swapb);
			for(i = 0; i < ncn; i++){
				if(todo[i] == swapa)
					sendchip(swapa, cn[i]);
				if(todo[i] == swapb)
					sendchip(swapb, cn[i]);
			}
		} else {
			if (dbfp) fprintf(dbfp, "no improvement\n");
			break;
		}
		sigdisable();
 		wfile(buf);
		put1(IMPROVES);
		putl(-wwraplen);
		sigenable();
		flush();
	}
	sigdisable();
	put1(IMPROVES);
	putl(wwraplen);
	flush();
}

void
improv1(Chip *a, Chip *b)
{
	if(a->type->pkg->class != b->type->pkg->class) return;
	if(chipswap(a, b))
		return;
	if(wwraplen < bestwraplen){
		bestwraplen = wwraplen;
		swapa = a; swapb = b;
		if (dbfp) fprintf(dbfp, "new bestwraplen=%d [%s<->%s]\n", bestwraplen, swapa->name, swapb->name);
	}
	chipswap(a, b);
}

int
chipswap(Chip *c1, Chip *c2)
{
	Point p1, p2;
	char r1, r2;
	int ret = 0;
#define		PL(c,p,r)	(c->pt=p, c->rotation=r, c->flags&=~UNPLACED, place(c))

	p1 = c1->pt; r1 = c1->rotation; unplace(c1);
	p2 = c2->pt; r2 = c2->rotation; unplace(c2);
	PL(c1, p2, r2);
	PL(c2, p1, r1);
	if((c2->flags|c1->flags)&UNPLACED){
		if(!(c1->flags&UNPLACED))
			unplace(c1);
		if(!(c2->flags&UNPLACED))
			unplace(c2);
		PL(c1, p1, r1);
		PL(c2, p2, r2);
		ret = 1;
	}
	updsig(c1);
	updsig(c2);
	rewrap(c1);
	rewrap(c2);
	return(ret);
}

void
rewrap(Chip *c)
{
	register Signal *s, **ss;
	register i;

	ss = (Signal **)symlook(c->name, S_CSMAP, (void *)0);
	while(s = *ss++)
		wrap(s);
}

void
sigdisable(void)
{
}

static
void
siggy(void)
{
	gothup = 1;
	if (dbfp) fprintf(dbfp, "sighup!\n");
}

void
sigenable(void)
{
}
