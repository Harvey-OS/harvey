#include	<u.h>
#include	<libc.h>
#include	<cda/fizz.h>

long wwraplen, wwires;
extern int oneok;
extern Signal *maxsig;
Board b;
int bflag, nflag, hflag, rflag, xflag;
int netno;

Drillsz *getdrill(char);
void getholes(Chip *);
void cntholes(Chip *);
void printholes(void);
void do_holes(void);
void prxlist(Signal *);
void prwlist(Signal *);
void prhlist(Chip *);
void output_border(Board);
output_xy(int, int);
Point nextseg(Point);

main(int argc, char *argv[])
{
	int n;
	extern long nph;
	extern int optind;

	while((n = getopt(argc, argv, "bhnrx")) != -1)
		switch(n)
		{
		case 'b':	bflag = 1; break;
		case 'n':	nflag = 1; break;
		case 'h':	hflag = 1; break;
		case 'r':	rflag = 1; break;
		case 'x':	xflag = 1; break;
		case '?':	break;
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(2, "%s: %d errors\n", *argv, n);
			exit(1);
		}
	if(n = fizzplace()){
		fprint(2, "%d chips unplaced\n", n);
		exit(1);
	}
	fizzplane(&b);
	if(fizzprewrap())
		exit(1);
	if (bflag) output_border(b);
	if (xflag) symtraverse(S_SIGNAL, prxlist);
	if (nflag) symtraverse(S_SIGNAL, prwlist);
	if (hflag) do_holes();
	exit(0);
}

void int_ch(void);
void getbholes(void);

void
do_holes(void)
{
		int i;
		int_ch();
		symtraverse(S_CHIP, cntholes);
		for(i = 0; i < b.ndrillsz; ++i)
			if(b.drillsz[i].count) {
				b.drillsz[i].pos = (Point *)
				   f_malloc(b.drillsz[i].count*(long)sizeof(Point));
				b.drillsz[i].pos += b.drillsz[i].count;
				}
		getbholes();
		symtraverse(S_CHIP, getholes);
		printholes();
}

void
int_ch(void)
{
	register j;
	Drillsz *ds;
	for(ds = b.drillsz; ds < &b.drillsz[b.ndrillsz]; ds++)
		ds->count == 0;
	for(j = 0, ds = b.drillsz; j < b.ndrills; j++) {
 		if(b.drills[j].drill != ds->letter)
			if((ds = getdrill(b.drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", b.drills[j].drill);
				exits("drill");
			}
		++ds->count;
	}
}

void
getbholes(void)
{
	register j;
	Drillsz *ds;
	for(j = 0, ds = b.drillsz; j < b.ndrills; j++) {
 		if(b.drills[j].drill != ds->letter)
			if((ds = getdrill(b.drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", b.drills[j].drill);
				exits("drill");
			}
		*(--ds->pos) = b.drills[j].p;
	}
}

void
printholes(void)
{
	int j,i;
	Point * pt;
	for(i = 0; i < b.ndrillsz; ++i) {
		if(b.drillsz[i].count == 0) continue;
		if(b.drillsz[i].type == 'a')
			fprint(1, "hole %d -drilled -size: %d -wired\n", i+1, b.drillsz[i].dia);
		else
			fprint(1, "hole %d -drilled -size: %d\n", i+1, b.drillsz[i].dia);
	}
	for(i = 0; i <	b.ndrillsz; ++i) {
		pt = b.drillsz[i].pos;
		for(j = 0; j < b.drillsz[i].count; ++j, ++pt)
			fprint(1, "Hole %d (%d %d)\n", i+1, pt->x, pt->y);
	}
}

void
getholes(Chip *c)
{
	register j;
	Drillsz *ds = b.drillsz;
	for(j = 0; j < c->npins; j++) {
		if(c->pins[j].drill != ds->letter)
			if((ds = getdrill(c->pins[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->pins[j].drill);
				exit(1);
			}
		*(--ds->pos) = c->pins[j].p;
	}
	for(j = 0; j < c->ndrills; j++) {
		if(c->drills[j].drill != ds->letter)
			if((ds = getdrill(c->drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->drills[j].drill);
				exit(1);
			}
		*(--ds->pos) = c->drills[j].p;
	}
}

Drillsz *
getdrill(char letter)
{
	Drillsz *ds = b.drillsz;
	Drillsz *end = &b.drillsz[b.ndrillsz];

	for(; ds < end; ds++)
		if(ds->letter == letter)
			return ds;
	return (Drillsz *) 0;
}

void
cntholes(Chip *c)
{
	register j;
	Drillsz *ds = b.drillsz;
	for(j = 0; j < c->npins; j++) {
		if(c->pins[j].drill != ds->letter)
			if((ds = getdrill(c->pins[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->pins[j].drill);
				exit(1);
			}
		++ds->count;
	}
	for(j = 0; j < c->ndrills; j++) {
		if(c->drills[j].drill != ds->letter)
			if((ds = getdrill(c->drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->drills[j].drill);
				exit(1);
			}
		++ds->count;
	}
}

void
prxlist(Signal *s)
{
	int nfields;
	int n;

	if((s->type & VSIG) || (s->type == SUBVSIG))
		return;
	n = s->n;
	if(n < 2) return;
	fprint(1, "Name %s %d\n", s->name, ++netno);
}

void
prwlist(Signal *s)
{
	int nfields;
	Coord *co;
	Pin *p;
	int n;

	nfields = 0;
	if((s->type & VSIG) || (s->type == SUBVSIG))
		return;
	co = s->coords;
	p = s->pins;
	n = s->n;
	if(n < 2) return;
	fprint(1, "Net %d", ++netno);
	for(n -= 1; n >= 0; n--, co++, p++) {
		if(nfields++ == 5) {
			fprint(1, "\nNet %d", netno);
			nfields = 0;
		}
		output_xy(p->p.x, p->p.y);
	}
	fprint(1, "\n");
}

void
prhlist(Chip *c)
{
	register j;
	int drillno, drillsize;

	fprint(1, "!%s\n", c->name);
	for(j = 0; j < c->npins; j++) {
		drillno = c->pins[j].drill - 'A';
		if (drillno > b.ndrillsz) {
			fprint(2, "drill %c isn't found in board\n", c->pins[j].drill);
			exit(1);
		}
		drillsize = b.drillsz[drillno].dia;
		fprint(1, "Hole %d", drillsize);
		output_xy(c->pins[j].p.x, c->pins[j].p.y);
		fprint(1, "\n");
	}
}

pptype(char *s)
{
	register i;

	for(i = 0; i < 6; i++)
		if(b.v[i].name && (strcmp(b.v[i].name, s) == 0))
			return('0'+i);
	return('_');
}

output_xy(int x, int y)
{
	fprint(1, " (%d %d)", x, y);
}

output_rect(Rectangle r)
{
	output_xy(r.min.x, r.min.y); output_xy(r.min.x, r.max.y);
	output_xy(r.max.x, r.max.y); output_xy(r.max.x, r.min.y);
	output_xy(r.min.x, r.min.y);
}
int
comp(int *a, int *b) 
{
	if(a < b) return(-1);
	if(a == b) return(0);
	return(1);
}
int nsegs;
Rectangle *segs;
void
output_border(Board b)
{
	int * brkpts, nbrkpts, y, i, min, max;
	Point P0, P1, P2, PEND;
	Plane *p;
	nsegs = 0;
	nbrkpts = 0;
	cutout(&b);
	segs = (Rectangle *) lmalloc((long)(4 * b.nkeepouts * sizeof(Rectangle)));
	brkpts = (int *)lmalloc((long) (2 * b.nkeepouts * sizeof(int)));
	for(i = 0, p = b.keepouts; i < b.nkeepouts; i++, p++) {
		if(p->sense < 0) continue;
		brkpts[nbrkpts++] = p->r.max.y;
		brkpts[nbrkpts++] = p->r.min.y;
	}
	if(nbrkpts == 0) {
		fprint(2, "no 'wires + ' definition\n");
		exit(1);
	}
	qsort((char *)brkpts, nbrkpts, sizeof(int), comp); 
	while(--nbrkpts > 0) {
		if((y = (brkpts[nbrkpts] - brkpts[nbrkpts-1])/2) == 0)
			continue;
		y += brkpts[nbrkpts-1];
		min = 100000;
		max = 0;
		for(i = 0, p = b.keepouts; i < b.nkeepouts; i++, p++) {
			if(p->sense < 0) continue;
			if(p->r.max.y < y) continue;
			if(p->r.min.y > y) continue;
			if(p->r.min.x < min) min = p->r.min.x;
			if(p->r.max.x > min) max = p->r.max.x;
		}
		segs[nsegs].min.x = min;
		segs[nsegs].max.x = min;
		segs[nsegs].min.y = brkpts[nbrkpts-1];
		segs[nsegs++].max.y = brkpts[nbrkpts];
		segs[nsegs].min.x = max;
		segs[nsegs].max.x = max;
		segs[nsegs].min.y = brkpts[nbrkpts-1];
		segs[nsegs++].max.y = brkpts[nbrkpts];
	}
	nbrkpts = 0;
	for(i = 0, p = b.keepouts; i < b.nkeepouts; i++, p++) {
		if(p->sense < 0) continue;
		brkpts[nbrkpts++] = p->r.max.x;
		brkpts[nbrkpts++] = p->r.min.x;
	}
	qsort((char *)brkpts, nbrkpts, sizeof(int), comp); 
	while(--nbrkpts > 0) {
		if((y = (brkpts[nbrkpts] - brkpts[nbrkpts-1])/2) == 0)
			continue;
		y += brkpts[nbrkpts-1];
		min = 100000;
		max = 0;
		for(i = 0, p = b.keepouts; i < b.nkeepouts; i++, p++) {
			if(p->sense < 0) continue;
			if(p->r.max.x < y) continue;
			if(p->r.min.x > y) continue;
			if(p->r.min.y < min) min = p->r.min.y;
			if(p->r.max.y > min) max = p->r.max.y;
		}
		segs[nsegs].min.y = min;
		segs[nsegs].max.y = min;
		segs[nsegs].min.x = brkpts[nbrkpts-1];
		segs[nsegs++].max.x = brkpts[nbrkpts];
		segs[nsegs].min.y = max;
		segs[nsegs].max.y = max;
		segs[nsegs].min.x = brkpts[nbrkpts-1];
		segs[nsegs++].max.x = brkpts[nbrkpts];
	}
	fprint(1, "Border 0");
	P0.x = segs[0].min.x;
	P0.y = segs[0].min.y;
	PEND = P0;
	P1.x = segs[0].max.x;
	P1.y = segs[0].max.y;
	segs[0].min.x = -1;
	for(i = 1; i < nsegs; ++i) {
		P2 = nextseg(P1);
		if((P2.x == P0.x) || (P2.y == P0.y))
			P1 = P2;
		else {
			output_xy(P0.x, P0.y);
			P0 = P1;
			P1 = P2;
		}
	}
	P2 = PEND;
	if((P2.x == P0.x) || (P2.y == P0.y))
		P1 = P2;
	else {
		output_xy(P0.x, P0.y);
		P0 = P1;
		P1 = P2;
	}
	output_xy(P0.x, P0.y);
	output_xy(P1.x, P1.y);
	fprint(1, "\n");
	for(i = 0, p = b.keepouts, y = 1; i < b.nkeepouts; i++, p++) {
		if(p->sense > 0) continue;
		fprint(1, "Border %d", y++);
		output_rect(p->r);
		fprint(1, "\n");
	}
}
		
Point
nextseg(Point P)
{
	int i;
	Point Q;
	for(i = 0 ; i < nsegs; ++i) {
		if(segs[i].min.x == -1) continue;
		if((segs[i].min.x == P.x)
			&& (segs[i].min.y == P.y)) {
			Q.x = segs[i].max.x;
			Q.y = segs[i].max.y;
			segs[i].min.y = -1;
			return(Q);
		}
		if((segs[i].max.x == P.x)
			&& (segs[i].max.y == P.y)) {
			Q.x = segs[i].min.x;
			Q.y = segs[i].min.y;
			segs[i].min.y = -1;
			return(Q);
		}
	}
	fprint(2, "can't find seg\n");
	exit(1);
	return(Q);
}
