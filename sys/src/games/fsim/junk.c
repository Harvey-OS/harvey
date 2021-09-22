#include	"type.h"

long
fmul(Fract f, long l)
{
	long x;

	x = ((double)f / (double)ONE) * (double)l;
	return x;
}

Fract
fdiv(long n, long d)
{
	long x;

	x = ((double)n * (double)ONE) / (double)d;
	return x;
}

void
ringbell(void)
{
}

int
button(int b)
{

	if(ecanmouse())
		mouse = emouse();
	return mouse.buttons & (1<<(b-1));
}

int	timef	= -1;
long
realtime(void)
{
	char buf[4*12];
	long t;

	if(timef < 0)
		timef = open("/dev/cputime", 0);
	if(timef < 0)
		exits("realtime");
	seek(timef, 0, 0);
	read(timef, buf, sizeof(buf));
	t = atoi(buf+2*12);
	return (t*6)/100;	/* 60th sec */
}

void
nap(void)
{
	sleep(17);
}

void
wexits(char *s)
{
	sleep(1000);
	fprint(2, "%s\n", s);
	exits(s);
}

void
d_clr(Image *i, Rectangle r)
{
	d_copy(i, r, display->white);
}

void
d_set(Image *i, Rectangle r)
{
	d_copy(i, r, display->black);
}

void
d_string(Image *i, Point p, Font *f, char *s)
{
	string(i, p, display->black, ZP, f, s);
}

void
d_segment(Image *i, Point p1, Point p2, int)
{
	line(i, p2, p1, 0,0,0, display->black, ZP);
}

void
d_point(Image *i, Point p, int)
{
	d_set(i, Rect(p.x,p.y, p.x+1, p.y+1));
}

void
d_circle(Image *i, Point p, int r, int)
{
	ellipse(i, p, r, r, 0, display->black, ZP);
}

void
d_copy(Image *d, Rectangle r, Image *s)
{
	draw(d, r, s, nil, s->r.min);
}

void
d_or(Image *d, Rectangle r, Image *s)
{
	draw(d, r, display->black, s, s->r.min);
}

Image*
d_balloc(Rectangle r, int)
{
	Image *i;

	i = allocimage(display, r, CMAP8, 0, DWhite);
	if(i == nil)
		wexits("allocimage");
	return i;
}

void
d_bfree(Image *i)
{
	if(i != nil)
		freeimage(i);
}

void
d_bflush(void)
{
	flushimage(display, 1);
}

void
d_binit(void)
{

	if(initdraw(0, 0, "fsim") < 0)
		wexits("initdraw");
	initdraw(0,0,0);
//	for(np = &nav[0]; np->name != nil; np++)
//		convertxy(np);
//	for(vp = &var[0]; vp->v != 0; vp++) {
//		convertxy(vp);
//		vp->v = DEG(360-12.9);
//	}
}
