#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
#include	<stdio.h>
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

static pkgbase, curpkg, npkgs;
int pcnts[2000];

static
int
closeto(Rectangle a, Rectangle b)
{
	register m;

	m = MAX(MAX(abs(a.min.x-b.min.x),abs(a.min.y-b.min.y)),
		MAX(abs(a.max.x-b.max.x),abs(a.max.y-b.max.y)));
	return(m < INCH/4);
}

static void
dopkg(Package *p)
{
	register i;
	register Package *pp;

	npkgs++;
	for(i = pkgbase; i < curpkg; i++){
		pp = (Package *)symlook("~", i, (void *)0);
		if(closeto(p->r, pp->r)){
/*		if((p->npins == pp->npins) && rinr(p->r, pp->r) && rinr(pp->r, p->r)){/**/
			symlook(p->name, i, (void *)p);
			p->class = i;
			return;
		}
	}
	symlook("~", curpkg, (void *)p);
	p->class = curpkg++;
}

void
pkgch(Chip *c)
{
	pcnts[c->type->pkg->class]++;
}

void
pkgclass(void *f)
{
	int i;
	FILE *fp;

	fp = (FILE *) f;
	pkgbase = 100;
	curpkg = pkgbase;
	npkgs = 0;
	symtraverse(S_PACKAGE, dopkg);
	fprintf(fp, "%d pkg classes from %d pkgs\n", curpkg-pkgbase, npkgs);
	for(i = pkgbase; i < curpkg; i++)
		pcnts[i] = 0;
	symtraverse(S_CHIP, pkgch);
	for(i = pkgbase; i < curpkg; i++)
		fprintf(fp, "%d ", pcnts[i]);
	fprintf(fp, "\n");
}
