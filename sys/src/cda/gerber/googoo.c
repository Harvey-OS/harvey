#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>

#define	ALLOC	4096

typedef struct Gerber	Gerber;

struct Gerber{
	Gerber *next;
	int	n;
	uchar	buf[ALLOC];
};

void	docmd(char*);
void	dofile(int);
void	dolist(void);
void	domouse(Mouse);
void	eventloop(void);
long	gootol(char*, char**);
uchar*	gspace(int);
void	label(Point, int, int);
void	line(Point, Point);
void	polyfill(Bitmap*, Point*, int, Fcode);
void	saveit(Point, int);
void	sdiscf(Point, int);
int	seginrect(Point, Point, Rectangle);
void	squadf(Point[4]);
void	srectf(Rectangle);
void	xgrid(int, int);
void	ygrid(int, int);

Gerber *head;
Gerber *tail;

int	debug;

char *	infile;
Biobuf	out;

int	dflag = 1;
int	gflag;
int	wflag;

Rectangle srect;
Point	origin;
Point	norm;
Rectangle rclip;
Rectangle limit;
int	apers[100];
int	draws[4];

int	aper;

void
main(int argc, char **argv)
{
	int i, infd;

	limit = Rect(9999999, 9999999, -1, -1);
	rclip = Rect(-9999999, -9999999, 9999999, 9999999);

	ARGBEGIN{
	case 'c':
		rclip.min.x = strtol(ARGF(), 0, 0);
		rclip.min.y = strtol(ARGF(), 0, 0);
		rclip.max.x = strtol(ARGF(), 0, 0);
		rclip.max.y = strtol(ARGF(), 0, 0);
		break;
	case 'w':
		origin.x = strtol(ARGF(), 0, 0);
		origin.y = strtol(ARGF(), 0, 0);
		++wflag;
		/* fall through */
	case 'd':
		dflag = 0;
		break;
	case 'g':
		++gflag;
		break;
	case 'D':
		++debug;
		break;
	}ARGEND
	if(argc > 0){
		infile = argv[0];
		infd = open(infile, OREAD);
		if(infd < 0){
			perror(infile);
			exits("open");
		}
	}else{
		infile = "/fd/0";
		infd = 0;
	}
	Binit(&out, 1, OWRITE);
	dofile(infd);
	close(infd);
	Bprint(&out, "limit = { %d %d %d %d }\n",
		limit.min.x, limit.min.y, limit.max.x, limit.max.y);
	for(i=0; i<sizeof apers/sizeof apers[0]; i++)
		if(apers[i] != 0)
			Bprint(&out, "aper %d\t%d\n", i, apers[i]);
	Bprint(&out, "line %d\n", draws[1]);
	Bprint(&out, "move %d\n", draws[2]);
	Bprint(&out, "flash %d\n", draws[3]);
	Bflush(&out);
	if(dflag){
		if(!wflag)
			origin = limit.min;
		eventloop();
		print("%s %s -w %d %d %s # %d %d\n", argv0,
			gflag ? "-g -d" : "-d",
			origin.x, origin.y, infile,
			origin.x+Dx(srect), origin.y+Dy(srect));
	}
	exits(0);
}

void
eventloop(void)
{
	Event e[1];

	binit(0, 0, 0);
	ereshaped(bscreenrect(0));
	einit(Emouse|Ekeyboard);

	for(;;)switch(event(e)){
	case Emouse:
		domouse(e->mouse);
		break;
	case Ekeyboard:
		return;
	}
}

#define BUTTON(b)	(m.buttons&(1<<(b-1)))

enum { Cold, Warm, Track };

void
domouse(Mouse m)
{
	static int state = Cold;
	static Point delta;

	switch(state){
	case Cold:
	case Warm:
		if(!ptinrect(m.xy, screen.r))
			state = Cold;
		else if(BUTTON(1))
			delta=m.xy, state=Track;
		else
			state = Warm;
		break;
	case Track:
		if(BUTTON(1))
			break;
		if(!ptinrect(m.xy, screen.r))
			state = Cold;
		else{
			state = Warm;
			delta = sub(m.xy, delta);
			origin = add(origin, Pt(-delta.x, delta.y));
			ereshaped(screen.r);
		}
		break;
	}
}

void
ereshaped(Rectangle r)
{
	screen.r = r;
	clipr(&screen, r);
	srect = inset(r, 5);
	srect.min.x += 16;
	bitblt(&screen, srect.min, &screen, srect, Zero);
	srect = inset(srect, font->height);
	clipr(&screen, srect);
	norm.x = srect.min.x - origin.x;
	norm.y = srect.max.y + origin.y;

	dolist();
	if(gflag){
		xgrid(50, 100);
		ygrid(50, 100);
	}
}

void
dofile(int fd)
{
	char *l, *p;
	Biobuf in;

	Binit(&in, fd, OREAD);
	while(l = Brdline(&in, '\n')){	/* assign = */
		l[BLINELEN(&in)-1] = 0;
		while(p = strchr(l, '*')){	/* assign = */
			*p++ = 0;
			docmd(l);
			l = p;
		}
	}
}

void
docmd(char *l)
{
	long x, y, g, d;

	if(debug)
		Bprint(&out, "%s\n", l);
	switch(*l){
	case 'G':
		g = strtol(++l, &l, 10);
		if(*l != 'D'){
			fprint(2, "G without D\n");
			exits("format");
		}
		if(g != 54){
			fprint(2, "G %d ?\n", g);
			exits("format");
		}
		d = strtol(++l, &l, 10);
		if(*l != 0){
			fprint(2, "more stuff after G, D\n");
			exits("format");
		}
		if(d < 0 || d > 99){
			fprint(2, "strange aperture %d\n", d);
			exits("format");
		}
		if(debug)
			Bprint(&out, "\taperture = %d\n", d);
		aper = d;
		*gspace(1) = d;
		break;
	case 'X':
		x = gootol(++l, &l);
		if(*l != 'Y'){
			fprint(2, "X without Y\n");
			exits("format");
		}
		y = gootol(++l, &l);
		if(*l != 'D'){
			fprint(2, "X, Y without D\n");
			exits("format");
		}
		d = strtol(++l, &l, 10);
		if(*l != 0){
			fprint(2, "more stuff after X, Y, D\n");
			exits("format");
		}
		if(d < 1 || d > 3){
			fprint(2, "D %d ?\n", d);
			exits("format");
		}
		if(debug){
			switch(d){
			case 1:
				Bprint(&out, "\tline to %d, %d\n", x, y);
				break;
			case 2:
				Bprint(&out, "\tmove to %d, %d\n", x, y);
				break;
			case 3:
				Bprint(&out, "\t*flash* %d, %d\n", x, y);
				break;
			}
		}
		if(ptinrect(Pt(x, y), rclip)){
			if(x < limit.min.x)
				limit.min.x = x;
			if(x > limit.max.x)
				limit.max.x = x;
			if(y < limit.min.y)
				limit.min.y = y;
			if(y > limit.max.y)
				limit.max.y = y;
			++apers[aper];
			++draws[d];
			saveit(Pt(x, y), d);
		}
		break;
	}
}

uchar *
gspace(int k)
{
	Gerber *g;
	uchar *p;

	if(!(g = tail) || g->n+k > ALLOC){	/* assign = */
		g = malloc(sizeof(Gerber));
		g->next = 0;
		g->n = 0;
		if(tail)
			tail->next = g;
		else
			head = g;
		tail = g;
	}
	p = &g->buf[g->n];
	g->n += k;
	return p;
}

void
saveit(Point pt, int d)
{
	uchar *p = gspace(5);

	p[0] = 0x80 | (d<<4) | ((pt.x>>14)&0x0c) | ((pt.y>>16)&0x03);
	p[1] = pt.x>>8;
	p[2] = pt.x;
	p[3] = pt.y>>8;
	p[4] = pt.y;
}

void
dolist(void)
{
	Gerber *g;
	uchar *p, *q;
	int d, k;
	Point pt, prev = Pt(0,0);

	for(g=head; g; g=g->next){
		p = g->buf;
		q = &g->buf[g->n];
		while(p < q){
			k = *p++;
			if(!(k & 0x80)){
				aper = k;
				continue;
			}
			d = (k>>4)&0x07;
			pt.x = (k&0x0c)<<14;
			pt.x |= p[0]<<8;
			pt.x |= p[1];
			if(pt.x & (1<<17))
				pt.x |= (~0)<<17;
			pt.y = (k&0x03)<<16;
			pt.y |= p[2]<<8;
			pt.y |= p[3];
			if(pt.y & (1<<17))
				pt.y |= (~0)<<17;
			pt.x = norm.x+pt.x;
			pt.y = norm.y-pt.y;
			switch(d){
			case 1:
				line(prev, pt);
				break;
			case 2:
				break;
			case 3:
				line(pt, pt);
				break;
			}
			prev = pt;
			p += 4;
		}
	}
}

static int widths[] = {
	1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 0
};

void
line(Point from, Point to)
{
	int hw, dx, dy;
	float ux, uy, u;
	Rectangle s;
	Point quad[4];

	if(aper < 10)				/* ? */
		return;
	else if(aper <= 19)			/* square aperture */
		hw = widths[aper-10]/2;
	else					/* round aperture */
		hw = (aper-20)/2;
	s.min.x = srect.min.x - hw;
	s.min.y = srect.min.y - hw;
	s.max.x = srect.max.x + hw+1;
	s.max.y = srect.max.y + hw+1;
	if(!seginrect(from, to, s))
		return;

	if(from.y > to.y || from.y == to.y && from.x > to.x){
		Point t = from;
		from = to;
		to = t;
	}
	dx = to.x - from.x;
	dy = to.y - from.y;			/* non-negative */
	if(aper <= 19){				/* square aperture */
		if(dx == 0 || dy == 0){		/* easy case */
			from.x -= hw;
			from.y -= hw;
			to.x += hw+1;
			to.y += hw+1;
			srectf(Rpt(from, to));
			return;
		}				/* else general case */
		u = sqrt(dx*dx + dy*dy);
		ux = hw*dx/u;
		uy = hw*dy/u;
							/* v = (-uy, ux) */
		quad[0].x = 0.5 + from.x - ux - uy;	/* -u+v */
		quad[0].y = 0.5 + from.y - uy + ux;
		quad[1].x = 0.5 + from.x - ux + uy;	/* -u-v */
		quad[1].y = 0.5 + from.y - uy - ux;
		quad[2].x = 0.5 + to.x   + ux + uy;	/* +u-v */
		quad[2].y = 0.5 + to.y   + uy - ux;
		quad[3].x = 0.5 + to.x   + ux - uy;	/* +u+v */
		quad[3].y = 0.5 + to.y   + uy + ux;
		squadf(quad);
		return;
	}					/* else round aperture */
	sdiscf(from, hw);
	if(dx == 0){				/* vertical */
		if(dy == 0)			/* single point */
			return;
		sdiscf(to, hw);
		from.x -= hw;
		to.x += hw+1;
		srectf(Rpt(from, to));
		return;
	}
	sdiscf(to, hw);
	if(dy == 0){				/* horizontal */
		from.y -= hw;
		to.y += hw+1;
		srectf(Rpt(from, to));
		return;
	}					/* else general case */
	u = sqrt(dx*dx + dy*dy);
	ux = hw*dx/u;
	uy = hw*dy/u;
						/* v = (-uy, ux) */
	quad[0].x = 0.5 + from.x - uy;		/* +v */
	quad[0].y = 1.5 + from.y + ux;
	quad[1].x = 0.5 + from.x + uy;		/* -v */
	quad[1].y = 0.5 + from.y - ux;
	quad[2].x = 1.5 + to.x   + uy;		/* -v */
	quad[2].y = 0.5 + to.y   - ux;
	quad[3].x = 0.5 + to.x   - uy;		/* +v */
	quad[3].y = 0.5 + to.y   + ux;
	squadf(quad);
}

#define	MIN(a, b)	((a)<=(b) ? (a) : (b))
#define	MAX(a, b)	((a)>=(b) ? (a) : (b))

int
seginrect(Point p, Point q, Rectangle r)
{
	int dx, dy;

	if(p.y > q.y || p.y == q.y && p.x > q.x){
		Point t = p;
		p = q;
		q = t;
	}
	if(r.min.y > q.y)
		return 0;
	if(p.y >= r.max.y)
		return 0;
	if(r.min.x > MAX(p.x, q.x))
		return 0;
	if(MIN(p.x, q.x) >= r.max.x)
		return 0;
	dx = q.x - p.x;
	dy = q.y - p.y;
	if(dx == 0 || dy == 0)
		return 1;
	if(dx > 0){
		if((r.max.y-p.y)*dx < (r.min.x-p.x)*dy)
			return 0;
		if((r.max.x-p.x)*dy < (r.min.y-p.y)*dx)
			return 0;
	}else{
		if((r.max.y-p.y)*dx > (r.max.x-p.x)*dy)
			return 0;
		if((r.min.x-p.x)*dy > (r.min.y-p.y)*dx)
			return 0;
	}
	return 1;
}

void
srectf(Rectangle r)
{
	bitblt(&screen, r.min, &screen, r, F);
}

void
sdiscf(Point p, int radius)
{
	disc(&screen, p, radius, ~0, F);
}

void
squadf(Point pp[4])
{
	polyfill(&screen, pp, 4, F);
}

void
label(Point pt, int av, int flag)
{
	int v;
	char *p, buf[32];
	Point s;

	v = av;
	p = buf;
	if(v < 0){
		v = -v;
		*p++ = '-';
	}
	sprint(p, "%d.%d", v/1000, (v/100)%10);
        s = strsize(font, buf);
	if(flag)
		pt.x -= s.x/2;
	else
		pt.y -= s.y/2;
	string(&screen, pt, font, buf, S);
}

void
xgrid(int mod, int lmod)
{
	int i, x, xmax;

	x = origin.x%mod;
	if(x < 0)
		x = -x;
	else if(x > 0)
		x = mod - x;
	x += norm.x + origin.x;
	xmax = norm.x + origin.x + Dx(srect);
	for(; x < xmax; x += mod){
		segment(&screen, Pt(x, srect.min.y), Pt(x, srect.max.y), ~0, F);
		i = x-norm.x;
		if(i%lmod == 0)
			label(Pt(x, srect.min.y), i, 1);
	}
}

void
ygrid(int mod, int lmod)
{
	int i, y, ymin;

	y = origin.y%mod;
	if(y < 0)
		y = -y;
	else if(y > 0)
		y = mod - y;
	y = norm.y - origin.y - y;
	ymin = norm.y - origin.y - Dy(srect);
	for(; y >= ymin; y -= mod){
		segment(&screen, Pt(srect.min.x, y), Pt(srect.max.x, y), ~0, F);
		i = norm.y-y;
		if(i%lmod == 0)
			label(Pt(srect.min.x, y), i, 0);
	}
}

long
gootol(char *p, char **pp)
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
