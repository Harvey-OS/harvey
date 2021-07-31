#include	<u.h>
#include	<libc.h>
#include	<cda/fizz.h>

#define		RAD(x)	(dofd(x)/2)

void draw(int dopts, int dovb);
static void drawsig(Signal *);
int dofd(char);
int chname;
int boxed;
int ruled = 1;
int framed;
Board b;
void pintraverse(void (*fn)(Pin ));

void
main(int argc, char **argv)
{
	int n;
	int dopts = 0;
	int dovb = 0;
	int dosigs = 0;
	extern int optind;

	chname = 0;
	while((n = getopt(argc, argv, "bfkprstv")) != -1)
		switch(n)
		{
		case 'k':	chname = 1; break;
		case 't':	chname = -1; break;
		case 'p':	dopts = 1; break;
		case 'v':	dovb = 1; break;
		case 'b':	boxed = 1; break;
		case 'r':	ruled = 0; break;
		case 'f':	framed = 1; break;
		case 's':	dosigs = 1; break;
		case '?':	break;
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(1, "%s: %d errors\n", *argv, n);
			exit(1);
		}
	fizzplane(&b);
	if(n = fizzplace()){
		/* complain but don't exit */
		fprint(2, "%d chips unplaced\n", n);
	}
	if(fizzprewrap())
		dosigs = 0;
	draw(dopts, dovb);
	if(dosigs){
		extern Signal *maxsig;
		extern void tsp(Signal *);

		symtraverse(S_SIGNAL, netlen);
		if(maxsig && ((maxsig->type & VSIG) != VSIG) && (maxsig->n >= MAXNET)){
			fprint(1, "net %s is too big (%d>=%d)\n", maxsig->name,
				maxsig->n, MAXNET);
			exits("bignet");
		}
		wwires = 0;
		wwraplen = 0;
		symtraverse(S_SIGNAL, tsp);
		symtraverse(S_SIGNAL, drawsig);
	}
	exit(0);
}

static void
drawsig(Signal *s)
{
	Pin *p;
	int i;

	if(s->type == VSIG) return;
	p = s->pins+1;
	for(i = s->n; i > 1; i--){
		fprint(1, "line %d %d %d %d\n", p[-1].p.x, p[-1].p.y, p->p.x, p->p.y);
		p++;
	}
}

static void
drawchip(Chip *c)
{
	int vert = (c->r.max.x-c->r.min.x < INCH/2) && (strlen(c->name) > 3);
	char *name;

	if(c->flags&UNPLACED)
		return;
	fprint(1, "poly { %d %d %d %d %d %d %d %d %d %d }\n", c->r.min.x,
		c->r.min.y, c->r.min.x, c->r.max.y,
		c->r.max.x, c->r.max.y, c->r.max.x,
		c->r.min.y, c->r.min.x, c->r.min.y);
	name = chname==0 ? c->name :
		chname<0 ? (c->type->code ? c->type->code : c->typename) :
		c->type->pkgname;
	if (boxed)
	fprint(1, "move %d %d\nbtext %d %d\ntext \\C%s\n",
		(c->r.min.x+c->r.max.x)/2,
		(c->r.min.y+c->r.max.y)/2,
		(c->r.max.x-c->r.min.x),
		(c->r.max.y-c->r.min.y),
		name);
	else
	fprint(1, "move %d %d\ntext \\C%s%s%s\n",
		(c->r.min.x+c->r.max.x)/2,
		(c->r.min.y+c->r.max.y)/2,
		vert? "\\b:" : "",
		name,
		vert? ":" : "");
	fprint(1, "circle %d %d %d\n", c->pt.x, c->pt.y, INCH/20);
}

static void
drawrule(void)
{
	register i;

	for(i = b.prect.min.x; i < b.prect.max.x; i += INCH/10){
		if((i%INCH) == 0){
			fprint(1, "line %d %d %d %d\n", i, b.prect.min.y, i, b.prect.min.y-50*(INCH/100));
			fprint(1, "move %d %d\ntext \\R%d\n", i,
				b.prect.min.y-70*(INCH/100), i/INCH);
		} else if((i%(INCH/2)) == 0)
			fprint(1, "line %d %d %d %d\n", i, b.prect.min.y, i, b.prect.min.y-35*(INCH/100));
		else
			fprint(1, "line %d %d %d %d\n", i, b.prect.min.y, i, b.prect.min.y-25*(INCH/100));
	}
	for(i = b.prect.min.y; i < b.prect.max.y; i += INCH/10){
		if((i%INCH) == 0){
			fprint(1, "line %d %d %d %d\n", b.prect.min.x, i, b.prect.min.x-50*(INCH/100), i);
			fprint(1, "move %d %d\ntext \\R%d\n",
				b.prect.min.x-70*(INCH/100), i, i/INCH);
		} else if((i%50) == 0)
			fprint(1, "line %d %d %d %d\n", b.prect.min.x, i, b.prect.min.x-35*(INCH/100), i);
		else
			fprint(1, "line %d %d %d %d\n", b.prect.min.x, i, b.prect.min.x-25*(INCH/100), i);
	}
}

static void
drawpin(Pin p)
{
	fprint(1, "circle %d %d %d\n", p.p.x, p.p.y, RAD(p.drill));
}

void
drawvbpin(Pin *p, int n)
{
	double delta, theta, r;
	register i;

	n += 3;
	r = ((RAD(p->drill)+10)/2.0)/sin(PI/n);
	fprint(1, "poly {");
	for(delta = 2*PI/n, theta = PI/2, i = 0; i <= n; i++, theta += delta)
		fprint(1, " %g %g", p->p.x+r*cos(theta), p->p.y+r*sin(theta));
	fprint(1, " }\n");
}

static
drawalign(void)
{
	register i;

	for(i = 0; i < 4; i++)
		fprint(1, "ci %d %d %d\nci %d %d %d\n",
			b.align[i].x, b.align[i].y, INCH/10,
			b.align[i].x, b.align[i].y, INCH/6);
}

void
drawframe(void)
{
	fprint(1, "line %d %d %d %d\n", b.prect.min.x, b.prect.min.y, b.prect.min.x, b.prect.max.y);
	fprint(1, "line %d %d %d %d\n", b.prect.min.x, b.prect.max.y, b.prect.max.x, b.prect.max.y);
	fprint(1, "line %d %d %d %d\n", b.prect.max.x, b.prect.max.y, b.prect.max.x, b.prect.min.y);
	fprint(1, "line %d %d %d %d\n", b.prect.max.x, b.prect.min.y, b.prect.min.x, b.prect.min.y);
}

void
draw(int dopts, int dovb)
{
	register i, j;

	fprint(1, "open\n");
	fprint(1, "range %d %d %d %d\n", b.prect.min.x-INCH, b.prect.min.y-INCH,
		b.prect.max.x+INCH, b.prect.max.y+INCH);
	fprint(1, ".color FCW\n");
	fprint(1, ".color P6\n");
	if (ruled) {
		drawrule();
		drawalign();
	}
	if (framed) drawframe();
	symtraverse(S_CHIP, drawchip);
	if(dopts)
		pintraverse(drawpin);
	if(dovb)
		for(i = 0; i < 6; i++)
			for(j = 0; j < b.v[i].npins; j++)
				drawvbpin(&b.v[i].pins[j], i);

	fprint(1, "close\n");
}
