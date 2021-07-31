#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<cda/fizz.h>

static void	draw(int, int);
static void	drawalign(void);
static void	drawchip(Chip*);
static void	drawdatums(Pin*);
static void	drawpin(Pin);
static void	drawref(void);
static void	drawrule(Rectangle);
static void	drawvbpin(Pin*, int);
static int	drdiam(int);
static void	macros(char*);
static int	max(int, int);
static int	min(int, int);
static void	polymacro(int, double);
static void	reftodatum(Point p);
static void	drawwires(void);
static void	drawholes(void);

#define		RAD(x)	(drdiam(x)/2)

int	chname;
int	boxed, noheader, landscape, nortext;
int	pinradius, vbdrill;
int	dopts, dovb, docpins, dodatums;
char *	wfile;
char *	hfile;
Point	pref; double dref;
Board	b;
Biobuf	out;

void
main(int argc, char **argv)
{
	char *fname;
	int n;

	Binit(&out, 1, OWRITE);
	ARGBEGIN{
	case 'b':
		boxed = 1; break;
	case 'c':
		docpins = 1; break;
	case 'd':
		dodatums = 1; break;
	case 'H':
		noheader = 1; break;
	case 'k':
		chname = 1; break;	/* package */
	case 'l':
		landscape = 1; break;
	case 'p':
		dopts = 1; break;
	case 'r':
		nortext = 1; break;
	case 't':
		chname = -1; break;	/* type */
	case 'v':
		dovb = 1; break;
	case 'h':
		hfile = ARGF();		/* icon-style hole file */
		break;
	case 'w':
		wfile = ARGF();		/* icon-style wire file */
		break;
	default:
		fprint(2, "usage: %s [-cdHklprtv] [-h file] [-w file] [files...]\n", argv0);
		exits("usage");
	}ARGEND
	USED(argc);
	if(wfile || hfile)
		dodatums = 1;
	if(wfile && !hfile)
		docpins = 1;
	fizzinit();
	f_init(&b);
	if(*argv)
		fname = *argv++;
	else
		fname = "/fd/0";
	do{
		if(n = f_crack(fname, &b)){
			fprint(2, "%s: %d errors\n", fname, n);
			exits("crack");
		}
	}while(fname = *argv++);	/* assign = */
	fizzplane(&b);
	if(n = fizzplace()){
		fprint(2, "warning: %d chip%s unplaced\n",
			n, n==1?"":"s");
	}
	if(fizzprewrap())
		exits("prewrap");
	draw(dopts, dovb);
	exits(0);
}

#define	Dx(r)	((r).max.x - (r).min.x)
#define	Dy(r)	((r).max.y - (r).min.y)
#define	Xy(p)	(p).x,(p).y

static void
reftodatum(Point p)
{
	int dx, dy;
	double d;
	if (!dodatums)
		return;
	dx = p.x - b.datums[0].p.x;
	dy = p.y - b.datums[0].p.y;
	d = sqrt((double)dx*dx + (double)dy*dy);
	if (dref > 0 && d > dref)
		return;
	dref = d;
	pref = p;
}

static void
drawpin(Pin p)
{
	int r = RAD(p.drill);
	if (r == 0)
		r = 27;
	if (pinradius != r)
		Bprint(&out, "circlerad=%d\n", pinradius=r);
	Bprint(&out, "pin(%d,%d)\n", Xy(p.p));
	reftodatum(p.p);
}

static void
drawchip(Chip *c)
{
	int vert, i;
	char *name;

	if(c->flags&UNPLACED)
		return;
	if(!wfile){
		vert = (nortext == 0) && (Dy(c->r) > Dx(c->r));
		name = chname==0 ? c->name :
			chname<0 ? (c->type->code ? c->type->code : c->typename) :
			c->type->pkgname;
		Bprint(&out, "chip(%s,%d,%d,%d,%d,%d,%d,%d)\n",
			name, Xy(c->pt), Dx(c->r), Dy(c->r),
			c->pt.x-c->r.min.x, c->pt.y-c->r.min.y, vert);
	}
	if (!docpins)
		return;
	for (i=0; i<c->npins; i++)
		drawpin(c->pins[i]);
	for (i=0; i<c->ndrills; i++)
		drawpin(c->drills[i]);
}
static void
drawdatums(Pin *d)
{
	int i;
	char *c1, *c2;
	for (i=0; i<3; i++) {
		switch (d[i].drill) {
		case '\\':
			c1 = ".se", c2 = ".nw"; break;
		case '/':
			c1 = ".ne", c2 = ".sw"; break;
		default:
			c1 = ".c", c2 = ".c"; break;
		}
		Bprint(&out, "box wid %d ht %d with %s at %d,%d\n",
			INCH/15, INCH/15, c1, Xy(d[i].p));
		Bprint(&out, "box wid %d ht %d with %s at %d,%d\n",
			INCH/15, INCH/15, c2, Xy(d[i].p));
	}
}

static void
drawref(void)
{
	if (wfile || !dodatums || dref <= 0)
		return;
	Bprint(&out, "arrowwid=30; arrowht=60\n");
	Bprint(&out, "line dotted 25 <- from %d,%d to %d,%d\n",
		b.datums[0].p.x, pref.y, pref.x, pref.y);
	Bprint(&out, "\"\\v'.8m'%.3f\\\"\\v'-.8m'\" center at last line.c\n",
		(double)(pref.x - b.datums[0].p.x)/1000.0);
	Bprint(&out, "line dotted 25 <- from %d,%d to %d,%d\n",
		pref.x, b.datums[0].p.y, pref.x, pref.y);
	Bprint(&out, "\"%.3f\\\"\" rjust at last line.c\n",
		(double)(pref.y - b.datums[0].p.y)/1000.0);
}

static void
drawrule(Rectangle r)
{
	int i, l;
	int label;

	r.min.x = (r.min.x/(INCH/10))*(INCH/10);
	r.min.y = (r.min.y/(INCH/10))*(INCH/10);

	for (i=r.min.x; i<r.max.x; i+=INCH/10) {
		label = 0;
		switch (i%INCH) {
		case 0:
			l = 3*(INCH/10); ++label; break;
		case INCH/2:
			l = 2*(INCH/10); break;
		default:
			l = 1*(INCH/10); break;
		}
		Bprint(&out, "line from (%d,%d) down %d\n",
			i, r.min.y, l);
		if (label)
			Bprint(&out, "\"%d\" below\n", i/INCH);
	}
	for (i=r.min.y; i<r.max.y; i+=INCH/10) {
		label = 0;
		switch (i%INCH) {
		case 0:
			l = 3*(INCH/10); ++label; break;
		case INCH/2:
			l = 2*(INCH/10); break;
		default:
			l = 1*(INCH/10); break;
		}
		Bprint(&out, "line from (%d,%d) left %d\n",
			r.min.x, i, l);
		if (label)
			Bprint(&out, "\"%d\" rjust\n", i/INCH);
	}
}

static void
polymacro(int n, double r)
{
	double theta;
	int i;

	Bprint(&out, "define poly%d %% [\n", n);
	for(i = 0; i <= n; i++) {
		theta = (((4*i+n)%(4*n))*(PI/2))/n;
		Bprint(&out, "\t%s %g,%g \\\n",
			i?"to":"line from",
			r*cos(theta), r*sin(theta));
	}
	Bprint(&out, "] at $1,$2\n%%\n");
}

static void
drawvbpin(Pin *p, int n)
{
	n += 3;
	if (vbdrill != p->drill) {
		vbdrill = p->drill;
		polymacro(n, RAD(vbdrill)/cos(PI/(2*n)));
	}
	Bprint(&out, "poly%d(%d,%d)\n", n, Xy(p->p));
}

static void
drawalign(void)
{
	int i, j;

	for(i=0; i<4; i++)
		for (j=0; j<2; j++) {
			Bprint(&out, "circle rad %d at %d,%d\n",
				INCH/(6+4*j), Xy(b.align[i]));
			if (i||j)
				continue;
			Bprint(&out, "line from last circle.w to last circle.e\n");
			Bprint(&out, "line from last circle.n to last circle.s\n");
		}
}

static void
macros(char *m)
{
	Bwrite(&out, m, strlen(m));
}

static void
draw(int dopts, int dovb)
{
	extern char trmacros[];
	extern char picmacros[];
	Rectangle rrect;
	int i, j;

	if (landscape) {
		Bprint(&out, "\\X:L 1:\n");
		Bprint(&out, ".ll 11i-\\n(.ou\n");
	}
	if (!noheader) {
		Bprint(&out, ".sp %d\n.ps 10\n.ft H\n", landscape?2:4);
		Bprint(&out, ".ce\n\\&%s\n", wfile ? wfile : b.name);
		Bprint(&out, ".sp 2\n");
	}
	macros(trmacros);
	Bprint(&out, ".PS %d.0\n", landscape?9:6);
	if (landscape) {
		Bprint(&out, "maxpswid=10.0\n");
		Bprint(&out, "maxpsht=8.0\n");
	}
	Bprint(&out, ".nr VS \\n(.v\n.nr PQ \\n(.f\n.nr PS \\n(.s\n");
	Bprint(&out, ".vs 6\n.ft CW\n.ps 6\n");
	macros(picmacros);

	if (dodatums) {
		rrect.min.x = min(b.datums[0].p.x,b.datums[1].p.x);
		rrect.min.x = min(b.datums[2].p.x,rrect.min.x);
		rrect.max.x = max(b.datums[0].p.x,b.datums[1].p.x);
		rrect.max.x = max(b.datums[2].p.x,rrect.max.x);
		rrect.min.y = min(b.datums[0].p.y,b.datums[1].p.y);
		rrect.min.y = min(b.datums[2].p.y,rrect.min.y);
		rrect.max.y = max(b.datums[0].p.y,b.datums[1].p.y);
		rrect.max.y = max(b.datums[2].p.y,rrect.max.y);
		Bprint(&out, "box wid %d ht %d with .sw at %d,%d\n",
			Dx(rrect), Dy(rrect), Xy(rrect.min));
		drawdatums(b.datums);
	} else {
		rrect = b.prect;
		drawalign();
	}
	rrect.min.x -= 2*INCH/10;
	rrect.min.y -= 2*INCH/10;
	rrect.max.x += 2*INCH/10;
	rrect.max.y += 2*INCH/10;
	drawrule(rrect);

	symtraverse(S_CHIP, drawchip);
	if(dopts) {
		pinradius = -1;
		pintraverse(drawpin);
	}
	if(dovb)
		for(i = 0; i < 6; i++) {
			vbdrill = 0;
			for(j = 0; j < b.v[i].npins; j++)
				drawvbpin(&b.v[i].pins[j], i);
		}
	if(wfile)
		drawwires();
	if(hfile)
		drawholes();
	drawref();
	Bprint(&out, ".vs \\n(VSu\n.ft \\n(PQ\n.ps \\n(PS\n");
	Bprint(&out, ".PE\n");
}

static int
drdiam(int c)
{
	static int drills[] = {
		'A', 33,
		'B', 34,
		'C', 39,
		'D', 42,
		'E', 50,
		'F', 62,
		'G', 106,
		'H', 107,
		'I', 108,
		'J', 20,
		'K', 110,
		'L', 111,
		'M', 112,
		'N', 113,
		'O', 114,
		'P', 115,
		'Q', 116,
		'R', 117,
		'S', 118,
		'T', 119,
		'U', 100,
		'V', 20,
		'W', 122,
		'X', 123,
		'Y', 124,
		'Z', 125,
		0, 0
	};
	int i;
	for (i=0; drills[i]; i+=2)
		if (c == drills[i])
			return drills[i+1];
	return 0;
}

static int
min(int a, int b)
{
	return a < b ? a : b;
}

static int
max(int a, int b)
{
	return a > b ? a : b;
}

static void
drawwires(void)
{
	Biobuf *wf;
	char *l;
	int tag, x, y;

	wf = Bopen(wfile, OREAD);
	if(wf == 0){
		perror(wfile);
		return;
	}
	while(l = Brdline(wf, '\n')){	/* assign = */
		tag = strtol(l, &l, 10);
		if(tag < 0)
			continue;
		x = strtol(l, &l, 10);
		y = strtol(l, &l, 10);
		Bprint(&out, "%s to %d,%d\n", (tag==5?"move":"line"), x, y);
	}
	Bclose(wf);
}

static void
drawholes(void)
{
	Biobuf *wf;
	char *l;
	int tag, x, y, r;

	wf = Bopen(hfile, OREAD);
	if(wf == 0){
		perror(hfile);
		return;
	}
	while(l = Brdline(wf, '\n')){	/* assign = */
		tag = strtol(l, &l, 10);
		if(tag < 0)
			continue;
		x = strtol(l, &l, 10);
		y = strtol(l, &l, 10);
		r = (strtol(l, &l, 10)+1)/2;
		if(pinradius != r)
			Bprint(&out, "circlerad=%d\n", pinradius=r);
		Bprint(&out, "pin(%d,%d)\n", x, y);
	}
	Bclose(wf);
}
