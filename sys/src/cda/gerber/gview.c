#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<libg.h>
#include	"gerber.h"

Biobuf stdout;
int debug = 0;
Rectangle limit;
Rectangle drect;

enum { INSTR_INC = 500 };
typedef enum { op_aperature, op_line, op_move, op_flash } Opcode;
typedef struct {
	Opcode op;
	Point p;
} Instr;
Instr *base;
int nexti;
int ninstr;
struct {
	int dia;
	enum { Round = 1, Square } shape;
} apertures[MAXAPER-MINAPER] = {
	[APER1] { 1, Round },
	[APER5] { 5, Round },
	[APER10] { 10, Round },
};
static Bitmap *grey, *greyp;

static Instr *dofile(char *);
static void getapertures(char *);
void docmd(char *, char *, int);
void addinstr(Opcode, Point);
long gtol(char *, char **);
void display(Instr *, Instr *, Bitmap *, int);
void setscale(Rectangle, Rectangle, int);
Point scale_point(Point);
Rectangle scale_rect(Rectangle);
int scale_scalar(int);
void connect(Bitmap *, Point, Point, int, int);
void polyfill(Bitmap *bp, int n, Point *p, int);

enum { Scale_reflectx = 1, Scale_exact = 2, };

void
main(int argc, char **argv)
{
	int scaleflags = 0;
	struct {
		int col;
		int scaleflags;
		char *file;
		Instr *code, *ecode;
	} args[20];
	int argi;
	int col = ~0;
	Bitmap *bp;
	int i;

	Binit(&stdout, 1, OWRITE);
	argi = 0;
	ARGBEGIN{
	case 'e':	scaleflags |= Scale_exact; break;
	case 'r':	scaleflags |= Scale_reflectx; break;
	case 'R':	scaleflags &= ~Scale_reflectx; break;
	case 'b':	args[argi].col = col = ~0;
			args[argi].scaleflags = scaleflags;
			args[argi++].file = ARGF();
			break;
	case 'g':	args[argi].col = col = 1;
			args[argi].scaleflags = scaleflags;
			args[argi++].file = ARGF();
			break;
	case 'a':	getapertures(ARGF()); break;
	case 'D':	debug = 1; break;
	case '?':	fprint(2, "Usage: %s -a afile [-es] [-[bg] file] ...\n", argv0);
			exits("usage");
	}ARGEND
	binit(berror, (char *)0, argv0);
	if(*argv == 0 && argi == 0)
		*--argv = "/fd/0";
	while(*argv){
		args[argi].col = col;
		args[argi].scaleflags = scaleflags;
		args[argi++].file = *argv++;
	}
	limit = Rect(9999999, 9999999, -1, -1);
	for(i = 0; i < argi; i++){
		args[i].ecode = dofile(args[i].file);
		args[i].code = base;
	}
	if(debug)
		Bprint(&stdout, "%d instrs; limit = %R\n", nexti, limit);
	Bflush(&stdout);
	bp = &screen;
	drect = screen.r;
	drect.min.x += 20;
	drect.max.x -= 10;
	drect.min.y += 10;
	drect.max.y -= 10;
	greyp = balloc((Rectangle){(Point){0, 0}, (Point){1, 1}}, 1);
	if(greyp == 0){
		fprint(2, "greyp balloc failed\n");
		exits("greyp balloc");
	}
	grey = balloc((Rectangle){(Point){0, 0}, (Point){32, 32}}, 1);
	if(grey == 0){
		fprint(2, "grey balloc failed\n");
		exits("grey balloc");
	}
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	for(i = 0; i < argi; i++){
		setscale(drect, limit, args[i].scaleflags);
		display(args[i].code, args[i].ecode, bp, args[i].col);
	}
	exits(0);
}

static void
getapertures(char *file)
{
	Biobuf *bp;
	char *s;
	int n;

	if((bp = Bopen(file, OREAD)) == 0){
		fprint(2, "%s: %s: \r\n", argv0, file);
		exits("get apertures fail");
	}
	while(s = Brdline(bp, '\n')){
		n = (s[1]-'0')*10 + (s[2]-'0');
		if((n < MINAPER) || (n >= MAXAPER)){
			fprint(2, "%s: bad aperture %c%c\n", s[1], s[2]);
			continue;
		}
		n -= MINAPER;
		apertures[n].dia = strtol(s+3, 0, 10);
		switch(s[17])
		{
		case 'r': case 'R':
			apertures[n].shape = Round; break;
		case 's': case 'S':
			apertures[n].shape = Square; break;
		default:
			fprint(2, "%s: bad aperture shape %c\n", s[17]);
			apertures[n].dia = 0;
		}
	}
	Bclose(bp);
}

static Instr *
dofile(char *file)
{
	char *l, *p;
	Biobuf *in;
	int lineno;

	ninstr = INSTR_INC;
	base = malloc(ninstr*sizeof(Instr));
	nexti = 0;
	if((in = Bopen(file, OREAD)) == 0){
		fprint(2, "%s: %r\n", file);
		exits("file open");
	}
	lineno = 1;
	while(l = Brdline(in, '\n')){	/* assign = */
		l[BLINELEN(in)-1] = 0;
		while(p = strchr(l, '*')){	/* assign = */
			*p++ = 0;
			docmd(l, file, lineno);
			l = p;
		}
		lineno++;
	}
	Bclose(in);
	return base+nexti;
}

void
docmd(char *l, char *file, int line)
{
	long g, d;
	Point p;
	static aper = -1;

	if(debug)
		Bprint(&stdout, "%s\n", l);
	switch(*l){
	case 'G':
		g = strtol(++l, &l, 10);
		if(*l != 'D'){
			fprint(2, "%s:%d: G without D\n", file, line);
			exits("format");
		}
		if(g != 54){
			fprint(2, "%s:%d: G %d ?\n", file, line, g);
			exits("format");
		}
		d = strtol(++l, &l, 10);
		if(*l != 0){
			fprint(2, "%s:%d: more stuff after G, D\n", file, line);
			exits("format");
		}
		if(d < MINAPER || d >= MAXAPER){
			fprint(2, "%s:%d: out of range aperture %d\n", file, line, d);
			exits("format");
		}
		if(apertures[d-MINAPER].dia == 0){
			fprint(2, "%s:%d: undefined aperture %d\n", file, line, d);
			exits("format");
		}
		p.x = apertures[d-MINAPER].dia;
		p.y = apertures[d-MINAPER].shape;
		if(debug)
			Bprint(&stdout, "\taperture = %d, shape = %d\n", p.x, p.y);
		aper = d;
		addinstr(op_aperature, p);
		break;
	case 'X':
		p.x = gtol(++l, &l);
		if(*l != 'Y'){
			fprint(2, "%s:%d: X without Y\n", file, line);
			exits("format");
		}
		p.y = gtol(++l, &l);
		if(*l != 'D'){
			fprint(2, "%s:%d: X, Y without D\n", file, line);
			exits("format");
		}
		d = strtol(++l, &l, 10);
		if(*l != 0){
			fprint(2, "%s:%d: more stuff after X, Y, D\n", file, line);
			exits("format");
		}
		if(d < 1 || d > 3){
			fprint(2, "%s:%d: D %d ?\n", file, line, d);
			exits("format");
		}
		switch(d){
		case 1:
			if(debug)
				Bprint(&stdout, "\tline to %P\n", p);
			addinstr(op_line, p);
			break;
		case 2:
			if(debug)
				Bprint(&stdout, "\tmove to %P\n", p);
			addinstr(op_move, p);
			break;
		case 3:
			if(debug)
				Bprint(&stdout, "\t*flash* %P\n", p);
			addinstr(op_flash, p);
			break;
		}
		if(p.x-aper/2 < limit.min.x)
			limit.min.x = p.x-aper/2;
		if(p.x+aper/2 > limit.max.x)
			limit.max.x = p.x+aper/2;
		if(p.y-aper/2 < limit.min.y)
			limit.min.y = p.y-aper/2;
		if(p.y+aper/2 > limit.max.y)
			limit.max.y = p.y+aper/2;
	}
}

long
gtol(char *p, char **pp)
{
	long factor = 10000, val = 0;
	int c;

	if(*p == '-'){
		factor = -factor;
		p++;
	}
	for(; c = *p; p++){	/* assign = */
		if(c < '0' || c > '9')
			break;
		val += (c - '0')*factor;
		factor /= 10;
	}
	if(*pp)
		*pp = p;
	return val;
}

void
addinstr(Opcode op, Point p)
{
	if(nexti >= ninstr){
		ninstr += INSTR_INC;
		if((base = realloc(base, ninstr*sizeof(Instr))) == 0)
			exits("realloc error");
	}
	base[nexti].op = op;
	base[nexti].p = p;
	nexti++;
}

void
display(Instr *i, Instr *e, Bitmap *bp, int col)
{
	Point cur, p;
	int aper;
	int special;

	special = 0;
	point(greyp, (Point){0,0}, col, S);
	texture(grey, grey->r, greyp, S);
	bflush();
	aper = -1;
	for(; i < e; i++) switch(i->op){
	case op_aperature:
		special = (i->p.x == 1);
		aper = scale_scalar(i->p.x);
		if(aper < 1)
			aper = 1;
		break;
	case op_move:
		cur = scale_point(i->p);
		break;
	case op_flash:
		cur = scale_point(i->p);
		connect(bp, cur, cur, aper, col);
		break;
	case op_line:
		p = scale_point(i->p);
		if(special)
			segment(bp, cur, p, col, S);
		else
			connect(bp, cur, p, aper, col);
		cur = p;
		break;
	}
	bflush();
}

void
connect(Bitmap *bp, Point p1, Point p2, int aper, int col)
{
	static Bitmap *b = 0;
	static int baper = -1;
	int w;
	double sx, sy, h;
	Point pt[4];
	Rectangle r;

	w = (aper+1)/2;
	if(baper != aper){
		if(b)
			bfree(b);
		baper = aper;
		b = balloc(Rect(-w, -w, w, w), 1);
		if(b == 0)
			exits("balloc fail");
		bitblt(b, b->r.min, b, b->r, Zero);
		disc(b, Pt(0, 0), w-1, col, S);
	}
	bitblt(bp, add(p1, b->r.min), b, b->r, DorS);
	if(eqpt(p1, p2))
		return;
	bitblt(bp, add(p2, b->r.min), b, b->r, DorS);
	if(p1.x == p2.x){
		if(p1.y < p2.y)
			r = Rect(p1.x-w, p1.y, p1.x-w+aper+1, p2.y+1);
		else
			r = Rect(p1.x-w, p2.y, p1.x-w+aper+1, p1.y+1);
		texture(bp, r, grey, DorS);
		return;
	}
	if(p1.y == p2.y){
		if(p1.x < p2.x)
			r = Rect(p1.x, p1.y-w, p2.x+1, p1.y-w+aper+1);
		else
			r = Rect(p2.x, p1.y-w, p1.x+1, p1.y-w+aper+1);
		texture(bp, r, grey, DorS);
		return;
	}
	sx = p2.x - p1.x;
	sy = p2.y - p1.y;
	h = w/hypot(sx, sy);
	sx = sx*h;
	sy = sy*h;
	pt[0] = add(p1, Pt(-sy, sx));
	pt[1] = add(p2, Pt(-sy, sx));
	pt[2] = add(p2, Pt(sy, -sx));
	pt[3] = add(p1, Pt(sy, -sx));
	if(debug)
		Bprint(&stdout, "fill: %P %P %P %P (%.2g, %.2g, %d %d)\n", pt[0], pt[1], pt[2], pt[3], sx, sy, w, aper);
	polyfill(bp, 4, pt, col);/**/
	segment(bp, pt[0], pt[1], col, S);
	segment(bp, pt[2], pt[3], col, S);
}

static double stretch;
static Point vo, so;
static int rx;

void
setscale(Rectangle s, Rectangle u, int flags)
{
	Point scr, world;

	scr = sub(s.max, s.min);
	world = sub(u.max, u.min);
	vo = u.min;
	stretch = scr.x/(double)world.x;
	if(stretch > scr.y/(double)world.y)
		stretch = scr.y/(double)world.y;
	if(flags&Scale_exact)
		stretch = .1;		/* screen is 100 dpi, board is .001 */
	if(debug)
		Bprint(&stdout, "stretch=%.2g; scr=%P, uni=%P\n", stretch, scr, world);
	if(flags&Scale_reflectx){
		so.x = s.max.x;
		rx = -1;
	} else {
		so.x = s.max.x - stretch*world.x;
		rx = 1;
	}
	so.y = s.max.y;
}

Point
scale_point(Point p)
{
	Point q;

	q.x = so.x + rx*(p.x-vo.x)*stretch+0.5;
	q.y = so.y - (p.y-vo.y)*stretch+0.5;
	return q;
}

Rectangle
scale_rect(Rectangle r)
{
	int q;

	r.min = scale_point(r.min);
	r.max = scale_point(r.max);
	if(r.min.x > r.max.x)
		q = r.min.x, r.min.x = r.max.x, r.max.x = q;
	if(r.min.y > r.max.y)
		q = r.min.y, r.min.y = r.max.y, r.max.y = q;
	return r;
}

int
scale_scalar(int x)
{
	x = x*stretch +0.5;
	return x;
}
