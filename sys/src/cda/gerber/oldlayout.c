#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<cda/fizz.h>
#include	"rtgraph.h"
#include	"gerber.h"

#define		TRACE		(INCH/20)
#define		PADANULUS	1.0
#define		GAP		(INCH/50)
#define		Maxpath		1000

static void chipgart(Chip *);
static void chipart(Chip *);
static void gchip(Chip *);
static void gschip(Chip *);
static void dosig(Signal *);
static void qsig(Signal *);
static void gsig(Signal *);
static void display(char *);
static Pin *drillp;
static int ndrills;
static void auxillary1(Chip *);
static void auxillary2(char *);
static int fillet(double *, int);
static void gerberpath(Pin *, int, int, int);
static void gerberpoint(Point);
static void gerberaper(int);
extern void gerberfill(double *pts, int n, int width);
extern void gerberstring(int, int, char *, int, int);
static void gerberrect(Rectangle r);
extern void gerbermoveto(int x, int y);
extern void gerberlineto(int x, int y);

Board b;
Rectangle arena;
int roundit = 0;
int arenax, arenay;
Biobuf stdout;
short drillmap[256];
int widths[26];
extern gerbconv(void *o, int f1, int f2, int f3, int chr);

#define	setwidth(q)	width=((q.drill>='A' && q.drill<='Z')? widths[q.drill-'A']:TRACE)/2

static int
isspace(int c)
{
	return((c == ' ') || (c == '\t'));
}

void
main(int argc, char **argv)
{
	int n;
	int quick = 0;
	int graphic = 0;
	int gerber = 0;
	int silk = 0;
	char *aux = 0;
	extern Signal *maxsig;
	extern void tsp(Signal *);
	extern void wrap(Signal *);
	extern int optind;
	extern char *optarg;

	Binit(&stdout, 1, OWRITE);
	for(n = 0; n < 26; n++)
		widths[n] = TRACE;
	fmtinstall('G', gerbconv);
	while((n = getopt(argc, argv, "gqrtSsa:G")) != -1)
		switch(n)
		{
		case 'a':	aux = optarg; break;
		case 'G':	gerber = 1; break;
		case 'g':	graphic = 1; break;
		case 'q':	quick = 1; break;
		case 'r':	roundit = 1; break;
		case 's':	silk = 1; break;
		case '?':	break;
		}
	if(quick || gerber)
		graphic = 0;
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	if(graphic){
		display(argv[optind]);
		exits(0);
	}
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(2, "%s: %d errors\n", *argv, n);
			exit(1);
		}
	fizzplane(&b);
	for(n = 0; n < b.nplanes; n++)
		if(strcmp(b.planes[n].sig, "analog_route") == 0)
			break;
	if(n >= b.nplanes){
		fprint(2, "no plane ``analog_route''\n");
		exits("no plane");
	}
	arena = b.planes[n].r;
	arenax = arena.max.x-arena.min.x;
	arenay = arena.max.y-arena.min.y;
	for(n = 0; n < b.ndrillsz; n++){
		if((b.drillsz[n].letter >= 'A') && (b.drillsz[n].letter <= 'Z'))
			widths[b.drillsz[n].letter - 'A'] = b.drillsz[n].dia;
		b.drillsz[n].count = 0;
	}
	if(n = fizzplace()){
		fprint(2, "%d chips unplaced\n", n);
		exits("unplaced");
	}
	if(n = fizzprewrap()){
		fprint(2, "%d prewrap errors\n", n);
		exits("prewrap errors");
	}
	symtraverse(S_SIGNAL, netlen);
	if(maxsig && ((maxsig->type & VSIG) != VSIG) && (maxsig->n >= MAXNET)){
		fprint(2, "net %s is too big (%d>=%d)\n", maxsig->name, maxsig->n, MAXNET);
		exits("bignet");
	}
	memset(drillmap, 0, sizeof drillmap);
	for(n = 0; n < b.ndrillsz; n++)
		drillmap[b.drillsz[n].letter] = b.drillsz[n].dia;
	if(quick){
		G_init(INCH, arena.min.x, arena.min.y, arena.max.x, arena.max.y);
		symtraverse(S_CHIP, chipgart);
		G_flush();
		symtraverse(S_SIGNAL, qsig);
		G_flush();
		G_exit();
	} else if(gerber){
		Pin pins[5];

		pins[0] = (Pin){(Point){arena.min.x, arena.min.y}, 'x', 0};
		pins[1] = (Pin){(Point){arena.max.x, arena.min.y}, 'x', 0};
		pins[2] = (Pin){(Point){arena.max.x, arena.max.y}, 'x', 0};
		pins[3] = (Pin){(Point){arena.min.x, arena.max.y}, 'x', 0};
		pins[4] = pins[0];
		gerberpath(pins, 5, 5, !silk);	/* (2nd) 5 wide track to indicate board outline */
		if(silk){
			ndrills = 0;
			if(aux)
				symtraverse(S_CHIP, auxillary1);
			if((drillp = malloc(ndrills*sizeof(*drillp))) == 0){
				fprint(2, "%s: malloc of %d drills failed: \r\n", argv0, ndrills);
				exits("malloc fail");
			}
			symtraverse(S_CHIP, gschip);
			if(aux)
				auxillary2(aux);
		} else {
			if(b.xydef){
				char *p, *q;
				int x, y;
	
				for(p = b.xydef; *p; ){
					while(isspace(*p))
						p++;
					x = strtol(p, &q, 10);
					p = q;
					while(isspace(*p))
						p++;
					y = strtol(p, &q, 10);
					p = q;
					while(isspace(*p))
						p++;
					q = p;
					while(*p){
						if(*p == '\n'){
							*p++ = 0;
							break;
						}
						p++;
					}
					gerberaper(10);	/* width of char lines */
					gerberstring(arena.max.x-x, y, q, 80, 2);
				}
			}
			symtraverse(S_SIGNAL, gsig);
			symtraverse(S_CHIP, gchip);
		}
	} else {
		Bprint(&stdout, "i %d %d %d %d %d\n", INCH, arena.min.x, arena.min.y, arena.max.x, arena.max.y);
		symtraverse(S_SIGNAL, dosig);
		symtraverse(S_CHIP, chipart);
	}
	exits(0);
}

static void
auxillary1(Chip *c)
{
	ndrills += c->npins + c->ndrills;
}

static
pcmp(Pin *a, Pin *b)
{
	int k;

	if(k = drillmap[a->drill]-drillmap[b->drill])
		return(k);
	if(k = a->p.y - b->p.y)
		return(k);
	return(a->p.x - b->p.x);
}

static void
auxillary2(char *stem)
{
	Biobuf *bp, *bp1;
	char buf[1024];
	int i;
	int lastdrill, drilldefs;
	Point lp;
	Pin *edrill, *p, *q;
	int dir;

	sprint(buf, "%sgerber", stem);
	if((bp = Bopen(buf, OWRITE)) == 0){
		fprint(2, "%s: %s: \r\n", argv0, buf);
		exits("aux1 fail");
	}
	for(i = 0; i < b.ndrillsz; i++)
		if((b.drillsz[i].letter >= 'A') && (b.drillsz[i].letter <= 'Z'))
			Bprint(bp, "G%d%8d mils %s\n", b.drillsz[i].letter - 'A' + 10,
				b.drillsz[i].dia, (b.drillsz[i].type=='r')? "round":"square");
	Bclose(bp);
	drillp -= ndrills;
	qsort(drillp, ndrills, sizeof(*drillp), pcmp);
	sprint(buf, "%sdrills", stem);
	if((bp = Bopen(buf, OWRITE)) == 0){
		fprint(2, "%s: %s: \r\n", argv0, buf);
		exits("aux2 fail");
	}
	sprint(buf, "%sdrills.def", stem);
	if((bp1 = Bopen(buf, OWRITE)) == 0){
		fprint(2, "%s: %s: \r\n", argv0, buf);
		exits("aux3 fail");
	}
	/*
		lets get fancy. print drills out boustrophodonically
	*/
	lastdrill = -1;
	drilldefs = 0;
	edrill = drillp+ndrills;
	dir = 1;	/* shut ken up */
	while(drillp < edrill){
		if(drillmap[drillp->drill] != lastdrill){
			lastdrill = drillmap[drillp->drill];
			drilldefs++;
			Bprint(bp1, "T%d=%4.3f\n", drilldefs, lastdrill*1e-3);
			Bprint(bp, "T%d\n", drilldefs);
			lp.x = lp.y = -1;
			dir = -1;
		}
		p = drillp;
		while((drillp < edrill) && (drillmap[drillp->drill] == lastdrill)
				&& (drillp->p.y == p->p.y))
			drillp++;
		/* p..drillp-1 is same size and same y */
		if(dir > 0){
			q = p; p = drillp-1; dir = -1;
		} else {
			q = drillp-1; dir = 1;
		}
		p -= dir;
		do {
			p += dir;
			if((p->p.x == lp.x) && (p->p.y == lp.y))
				continue;
			Bprint(bp, "%6d%6d\n", p->p.x, p->p.y);
			lp = p->p;
		} while(p != q);
	}
}

static void
dopin(Pin *p)
{
	Bprint(&stdout, "p %d %d %d %d\n", p->p.x, p->p.y, (int)(drillmap[p->drill]*(1+PADANULUS)), drillmap[p->drill]);
}

static void
chipart(Chip *c)
{
	int i;

	Bprint(&stdout, "c %s %d %d %d %d\n", c->name, c->r.min.x, c->r.min.y, c->r.max.x, c->r.max.y);
	for(i = 0; i < c->npins; i++)
		dopin(&c->pins[i]);
}

static void
gchip(Chip *c)
{
	int i;
	Pin *p;

	p = c->pins;
	for(i = 0; i < c->npins; i++, p++){
		gerberaper((int)(drillmap[p->drill]*(1+PADANULUS)));
		gerberpoint((Point){arena.max.x-p->p.x, p->p.y});
	}
	p = c->drills;
	for(i = 0; i < c->ndrills; i++, p++){
		gerberaper((int)(drillmap[p->drill]*(1+PADANULUS)));
		gerberpoint((Point){arena.max.x-p->p.x, p->p.y});
	}
}

/*
	coords are in package coords where offset to pin 1 is vx,vy.
	output coords are where pin1 is rx,ry
*/
static void
gerberartwork(int rx, int ry, int vx, int vy, int rot, char *art)
{
	int m[2][2];
	char *q;
	int move;
	int x, y, nx, ny;

	switch(rot)
	{
	case 0:	m[0][0]=1; m[0][1]=0; m[1][0]=0; m[1][1]=1; break;
	case 1:	m[0][0]=0; m[0][1]=-1; m[1][0]=1; m[1][1]=0; break;
	case 2:	m[0][0]=-1; m[0][1]=0; m[1][0]=0; m[1][1]=-1; break;
	case 3:	m[0][0]=0; m[0][1]=1; m[1][0]=-1; m[1][1]=0; break;
	}
	move = 1;
	gerberaper(1);
	while(*art){
		while(isspace(*art))
			art++;
		x = strtol(art, &q, 10);
		art = q;
		while(isspace(*art))
			art++;
		y = strtol(art, &q, 10);
		nx = rx + m[0][0]*(x+vx) + m[0][1]*(y+vy);
		ny = ry + m[1][0]*(x+vx) + m[1][1]*(y+vy);
		art = q;
		while(isspace(*art))
			art++;
		if(move)
			gerbermoveto(nx, ny);
		else
			gerberlineto(nx, ny);
		if(*art == '\n')
			move = 1, art++;
		else
			move = 0;
	}
}

static void
gschip(Chip *c)
{
	enum { Minheight = 20, Maxheight = 60 };
	Point dim;
	Pin *pp;
	int i, d;
	int ht;
	int rot;
	char buf[256];

	rot = c->rotation;
	gerberaper(1);
	gerberrect(c->r);
	dim.x = c->r.max.x - c->r.min.x;
	dim.y = c->r.max.y - c->r.min.y;
	if(c->comment)
		strcpy(buf, c->comment);
	else
		strcpy(buf, c->typename);
	if(rot&1){
		ht = dim.x/3;
		if(ht < Minheight) ht = Minheight;
		else if(ht > Maxheight) ht = Maxheight;
		gerberstring(c->r.min.x+dim.x/4, c->r.min.y+dim.y/2, c->name, ht, 3);
		gerberstring(c->r.min.x+3*dim.x/4, c->r.min.y+dim.y/2, buf, ht, 3);
	} else {
		ht = dim.y/3;
		if(ht < Minheight) ht = Minheight;
		else if(ht > Maxheight) ht = Maxheight;
		gerberstring(c->r.min.x+dim.x/2, c->r.min.y+dim.y/4, buf, ht, 0);
		gerberstring(c->r.min.x+dim.x/2, c->r.min.y+3*dim.y/4, c->name, ht, 0);
	}
	if(c->type->pkg->xydef)
		gerberartwork(c->pt.x, c->pt.y, c->type->pkg->xyoff.x,
			c->type->pkg->xyoff.y, c->rotation, c->type->pkg->xydef);
	for(i = 0, pp = c->pins; i < c->npins; i++, pp++){
		d = drillmap[pp->drill];
		gerberrect((Rectangle){(Point){pp->p.x-d/2, pp->p.y-d/2}, (Point){pp->p.x+d/2, pp->p.y+d/2}});
		*drillp++ = *pp;
	}
	for(i = 0, pp = c->drills; i < c->ndrills; i++, pp++){
		d = drillmap[pp->drill];
		gerberrect((Rectangle){(Point){pp->p.x-d/2, pp->p.y-d/2}, (Point){pp->p.x+d/2, pp->p.y+d/2}});
		*drillp++ = *pp;
	}
}

static void
chipgart(Chip *c)
{
	int i;
	Pin *p;
	Point ll;
	int pad_dia = INCH*.07;

	G_outline(c->r.min.x, c->r.min.y, c->r.max.x, c->r.max.y);
	p = c->pins;
	for(i = 0; i < c->npins; i++){
		ll = (Point){p->p.x-pad_dia/2, p->p.y-pad_dia/2};
		G_rect(ll.x, ll.y, ll.x + pad_dia, ll.y + pad_dia, Grey);
		p++;
	}
}

static void
qsig(Signal *s)
{
	Pin *p, *ep;
	int npins;

	if(s->layout == 0)
		return;
	p = s->layout->pins;
	npins = s->layout->npins;
	ep = s->layout->pins + npins;
	for(p++; p < ep; p++){
		G_line(p[-1].p.x, p[-1].p.y, p->p.x, p->p.y, Black);
	}
	G_flush();
}

static void
gsig(Signal *s)
{
	Pin *p;
	int width;

	if(s->layout == 0)
		return;
	p = s->layout->pins;
	setwidth(p[0]);
	if(width > MAXAPER)
		width = MAXAPER;
	gerberpath(p, s->layout->npins, width, 1);
}

static void
dosig(Signal *s)
{
	Pin *q, *p, *ep;
	int width, nc;
	struct crease { double dx, dy, d; int turn; } crease[Maxpath];
	double pts[4*Maxpath+4];
	struct crease *c;
	double *dp, *d;
	double dx, dy, ldx, ldy, h;
	double cx, cy;
	double d2x, d2y, mult;
	int npins;
	int debug;

#define	setslope(p1,p2)	(dx=p2.p.x-p1.p.x,dy=p2.p.y-p1.p.y,h=hypot(dx,dy),dx/=h,dy/=h)

	debug = strcmp(s->name, "Vs+") == 0;
	if(s->layout == 0)
		return;
	p = s->layout->pins;
	npins = s->layout->npins;
	ep = s->layout->pins + npins;
	nc = 0;
	setslope(p[0], p[1]);
	setwidth(p[0]);
	p[0].p.x -= dx*width; p[0].p.y -= dy*width;
	crease[nc] = (struct crease){-dy, dx, width, 0};
	nc++;
	for(q = p+2; q < ep; q++){
		ldx = dx;
		ldy = dy;
		setslope(q[-1], q[0]);
		setwidth(q[0]);
		/*
			first determine difference angle
		*/
		cx = dx*ldx + dy*ldy;
		cy = dy*ldx - dx*ldy;
if(debug) fprint(2, "D(%.2g/%.2g): ",cy,cx);/**/
		/* now quadrant, and therefore sign of cos(diff/2) */
		if((cy < 0) || ((cy == 0) && (cx < 0)))
			mult = -1;
		else
			mult = 1;
		if((h = 1-cx) < 0) h = 0;
		d2y = sqrt(h/2);
		if((h = 1+cx) < 0) h = 0;
		d2x = mult*sqrt(h/2);
		cy = mult*(ldx*d2x - ldy*d2y);
		cx = -mult*(ldy*d2x + ldx*d2y);
if(debug) fprint(2, "%d,%d %d,%d: ld=%.2g/%.2g, d=%.2g/%.2g, c=%.2g/%.2g c/2=%.2g/%.2g\n",q[-1].p.x,q[-1].p.y,q[0].p.x,q[0].p.y,ldy,ldx,dy,dx,cy,cx,d2y,d2x);
		/* adjust width */
		h = width/hypot(q[0].p.x-q[-1].p.x, q[0].p.y-q[-1].p.y);
		if(fabs(d2x) > h) h = fabs(d2x);
if(debug) fprint(2, "\th=%.2g, width=%.2g, '%c'\n", h, width/h, q->drill);
		crease[nc] = (struct crease){cx, cy, width/h, mult};
		nc++;
	}
	ep[-1].p.x += dx*width; ep[-1].p.y += dy*width;
	crease[nc] = (struct crease){-dy, dx, width, 0};
	nc++;
	USED(nc);

#define	WING(pt, m, cr)	*dp++ = pt.p.x+cr.dx*m*cr.d, *dp++ = pt.p.y+cr.d*m*cr.dy

	dp = pts;
	for(q = p, c = crease; q < ep; q++, c++){
		WING(q[0], 1, c[0]);
	}
	while(--q >= p){
		--c;
		WING(q[0], -1, c[0]);
	}
	Bprint(&stdout, "n %s %d", s->name, dp-pts);
	for(d = pts; d < dp; d++)
		Bprint(&stdout, " %.0f", *d);
	Bprint(&stdout, "\n");
}

static void
display(char *file)
{
	Biobuf *bp;
	int a, b, c, d, i;
	double db;
	int inch;
	double pt[2*Maxpath+4];

#define	getn(x)	(Bgetd(bp, &db), x = db+0.5)

	if((bp = Bopen(file, OREAD)) == 0){
		fprint(2, "%s: %r\n", file);
		return;
	}
	for(;;)switch(Bgetc(bp))
	{
	case 'i':
		getn(inch);
		getn(arena.min.x);
		getn(arena.min.y);
		getn(arena.max.x);
		getn(arena.max.y);
		G_init(inch, arena.min.x, arena.min.y, arena.max.x, arena.max.y);
		break;
	case 'n':
		while(isspace(Bgetc(bp)))
			;
		while(!isspace(Bgetc(bp)))
			;
		getn(a);
		for(i = 0; i < a; i++)
			Bgetd(bp, &pt[i]);
		if(roundit)
			a = fillet(pt, a);
		G_poly(pt, a, Black);
		Brdline(bp, '\n');
		break;
	case 'c':
		while(isspace(Bgetc(bp)))
			;
		while(!isspace(Bgetc(bp)))
			;
		getn(a);
		getn(b);
		getn(c);
		getn(d);
		G_outline(a, b, c, d);
		while((a = Bgetc(bp)) >= 0)
			if(a == '\n')
				break;
		break;
	case 'p':
		getn(a);
		getn(b);
		getn(c);
		getn(d);
		Brdline(bp, '\n');
		/*G_disc(a, b, c, Black);*/ USED(c);
		G_disc(a, b, d, White);
		break;
	case ' ':
	case '\t':
	case '\n':
		break;
	case -1:
		goto done;
	}
done:
	G_flush();
	G_exit();
}

typedef struct { double x, y; } Fpoint;

static int
spline(double x1, double y1, double x2, double y2, double x3, double y3, double *out)
{
	long w, t1, t2, t3, fac=1000; 
	int i, j, steps=10; 
	Fpoint p, q;
	int n;
	Fpoint pp[10];
	int ndone;

	pp[0] = (Fpoint){x1, y1};
	pp[1] = (Fpoint){(x1+2*x2+x3)/4, (y1+2*y2+y3)/4};
	pp[2] = (Fpoint){x3, y3};
	n = 3;
	for (i = n; i > 0; i--)
		pp[i] = pp[i-1];
	pp[n+1] = pp[n];
	n += 2;
	p = pp[0];
	ndone = 0;
	for(i = 0; i < n-2; i++)
	{
		for(j = 0; j < steps; j++)
		{
			w = fac * j / steps;
			t1 = w * w / (2 * fac);
			w = w - fac/2;
			t2 = 3*fac/4 - w * w / fac;
			w = w - fac/2;
			t3 = w * w / (2*fac);
			q.x = (t1*pp[i+2].x + t2*pp[i+1].x + 
				t3*pp[i].x + fac/2) / fac;
			q.y = (t1*pp[i+2].y + t2*pp[i+1].y + 
				t3*pp[i].y + fac/2) / fac;
			*out++ = p.x;
			*out++ = p.y;
			ndone += 2;
			p = q;
		}
	}
	*out++ = p.x;
	*out = p.y;
	ndone += 2;
	return(ndone);
}

static int
fillet(double *pt, int n)
{
	double mypt[Maxpath], *p;
	int g, i;
	double x, y, lx, ly, nx, ny;
	double dx, dy, ndx, ndy, h, d1, d2;

#define	setdir(x1,y1,x2,y2,dx,dy) (dx=x2-x1, dy=y2-y1, h=hypot(dx,dy), dx/=h, dy/=h)
#define	ADD(x,y)	(pt[g++]=(x), pt[g++]=(y))
#define	D	100

	memcpy(mypt, pt, n*sizeof(pt[0]));
	mypt[n] = pt[0];
	mypt[n+1] = pt[1];
	mypt[n+2] = pt[2];
	mypt[n+3] = pt[3];
	n /= 2;		/* number of pts */
	p = mypt;
	x = *p++;
	y = *p++;
	g = 0;
	for(i = 0; i < n; i++){
		lx = x;
		ly = y;
		x = *p++;
		y = *p++;
		nx = p[0];
		ny = p[1];
		setdir(lx, ly, x, y, dx, dy);
		d1 = h/2; if(d1 > D) d1 = D;
		setdir(x, y, nx, ny, ndx, ndy);
		d2 = h/2; if(d2 > D) d2 = D;
/*		ADD(x-dx*d1, y-dy*d1);
		ADD(x+ndx*d2, y+ndy*d2);
*/
		g += spline(x-dx*d1, y-dy*d1, x, y, x+ndx*d2, y+ndy*d2, &pt[g]);
	}
/*print("%d pt in: %.0f/%.0f  %.0f/%.0f  %.0f/%.0f  %.0f/%.0f ...  %.0f/%.0f %.0f/%.0f\n", 2*n, mypt[0], mypt[1], mypt[2], mypt[3], mypt[4], mypt[5], mypt[6], mypt[7], mypt[2*n-4], mypt[2*n-3], mypt[2*n-2], mypt[2*n-1]);
print("%d pts out: %.0f/%.0f  %.0f/%.0f  %.0f/%.0f  %.0f/%.0f ...  %.0f/%.0f %.0f/%.0f\n", g, pt[0], pt[1], pt[2], pt[3], pt[4], pt[5], pt[6], pt[7], pt[g-4], pt[g-3], pt[g-2], pt[g-1]);/**/
	return(g);
}

int
gerbconv(void *o, int f1, int f2, int f3, int chr)
{
	char buf[16];
	char *p = buf;

	USED(f1); USED(f2); USED(f3); USED(chr);
	p += sprint(p, "%.5d", *(long *)o);
	while(--p > buf && *p == '0')	/* sic dixit jhc */
		*p = 0;
	strconv(buf, 0, -1000, 0);
	return sizeof(long);
}

static void
gerberaper(int width)
{
	static int prevap;
	int apsel;

	apsel = width;
	if(apsel < MINAPER || apsel > MAXAPER){
/*		fprint(2, "bad path width %d", width);
		exits("bad aper");/**/
		apsel = (apsel > MAXAPER)? MAXAPER:MINAPER;
	}
	if(apsel != prevap){
		Bprint(&stdout, "G54D%d*\n", apsel);
		prevap = apsel;
	}
}

static void
gerberpath(Pin *pp, int n, int width, int reflectx)
{
	Pin *ep;
	Pin *sp;
	double pts[1000];
	int nd;
	int a, b;

	if(reflectx)
		a = -1, b = arena.max.x;
	else
		a = 1, b = 0;
	gerberaper(width);
	ep = pp+n;
	while(pp < ep){
		sp = pp;
		Bprint(&stdout, "X%GY%GD02*\n", pp->p.x*a+b, pp->p.y);
		for(pp++; pp < ep; pp++){
			Bprint(&stdout, "X%GY%GD01*\n", pp->p.x*a+b, pp->p.y);
			if(pp->drill == 'Z')
				break;
		}
		if(pp < ep){
			if((sp->p.x == pp->p.x) && (sp->p.y == pp->p.y)){
				nd = 0;
				while(sp < pp){
					pts[nd++] = sp->p.x*a+b;
					pts[nd++] = sp->p.y;
					sp++;
				}
				gerberfill(pts, nd, width);
			}
			pp++;
		}
	}
}

void
gerberhseg(int x1, int x2, int y)
{
	Bprint(&stdout, "X%GY%GD02*\n", x1, y);
	Bprint(&stdout, "X%GY%GD01*\n", x2, y);
}

void
gerbermoveto(int x, int y)
{
	Bprint(&stdout, "X%GY%GD02*\n", x, y);
}

void
gerberlineto(int x, int y)
{
	Bprint(&stdout, "X%GY%GD01*\n", x, y);
}

static void
gerberpoint(Point p)
{
	Bprint(&stdout, "X%GY%GD03*\n", p.x, p.y);
}

static void
gerberrect(Rectangle r)
{
	Bprint(&stdout, "X%GY%GD02*\n", r.min.x, r.min.y);
	Bprint(&stdout, "X%GY%GD01*\n", r.min.x, r.max.y);
	Bprint(&stdout, "X%GY%GD01*\n", r.max.x, r.max.y);
	Bprint(&stdout, "X%GY%GD01*\n", r.max.x, r.min.y);
	Bprint(&stdout, "X%GY%GD01*\n", r.min.x, r.min.y);
}
