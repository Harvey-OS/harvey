#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

static Plane *np, *cp, *npmax, *cpmax;

void
ccut(Chip *c)
{
	register i;
	register Package *p;

	p = c->type->pkg;
	for(i = 0; i < p->ncutouts; i++){
		*np = p->cutouts[i];
		planerot(&(np->r), c);
		np->r = fg_raddp(np->r, c->pt);
		if(np < npmax) np++;
		else
			fprint(2, "np >= npmax\n");
	}
	for(i = 0; i < p->nkeepouts; i++){
		*cp = p->keepouts[i];
		planerot(&(cp->r), c);
		cp->r = fg_raddp(cp->r, c->pt);
		if(cp < cpmax) cp++;
		else
			fprint(2, "cp >= cpmax\n");
	}
}

void
cutout(Board *b)
{
	Plane planes[100], kp[100];
	int n;

	np = planes;
	npmax = &planes[100-1];
	cp = kp;
	cpmax = &kp[100-1];
	symtraverse(S_CHIP, ccut);
	n = np-planes;
	if(b->planes == 0)
		b->planes = (Plane *) lmalloc((long) (n*sizeof(Plane)));
	else
		b->planes = (Plane *)Realloc((char *)b->planes, (n+b->nplanes)*sizeof(Plane));
	memcpy((char *)&b->planes[b->nplanes], (char *)planes, n*sizeof(Plane));
	b->nplanes += n;
	n = cp-kp;
	if(b->keepouts == 0)
		b->keepouts = (Plane *) lmalloc((long) (n*sizeof(Plane)));
	else
		b->keepouts = (Plane *)Realloc((char *)b->keepouts, (n+b->nkeepouts)*sizeof(Plane));
	memcpy((char *)&b->keepouts[b->nkeepouts], (char *)kp, n*sizeof(Plane));
	b->nkeepouts += n;
}
