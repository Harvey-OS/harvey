#include <u.h>
#include <libc.h>
#include <libg.h>

void	face(Bitmap *, Rectangle, int, int);
Point	limb(Point, int, int);

void
ignore(void *a, char *msg)
{
	USED(a, msg);
	noted(NCONT);
}

void
main(void)
{
	int ha, ma, f, n;
	Rectangle r;
	char buf[128];
	Bitmap *d;
	Tm *tv;

	binit(0, 0, "clock");
	clipr(&screen, screen.r);

	r = screen.r;

	tv = localtime(time(0));
	if(tv->hour > 12)
		tv->hour -= 12;

	ha = 90 - ((360/12) * tv->hour) - ((360 * tv->min)/(60*12));
	ma = 90 - ((360/60) * tv->min);
	d = balloc(r, 0);
	face(d, r, ha, ma);
	bitblt(&screen, r.min, d, r, S);
	bfree(d);

	notify(ignore);

	f = open("/dev/mouse", OREAD);
	if(f < 0){
		fprint(2, "can't open /dev/mouse: %r\n");
		exits(0);
	}
	alarm(1000*60);
	for(;;) {
		n = read(f, buf, sizeof(buf));
		alarm(1000*60);
		if(n < 0) {
			errstr(buf);
			if(strcmp(buf, "interrupted") != 0)
			if(strcmp(buf, "no error") != 0)
				exits(buf);
			buf[0] = 'm';
			buf[1] = 0x80;
		}
		if(buf[0] != 'm' || (buf[1]&0x80) == 0)
			continue;

		r = bscreenrect(0);
		screen.r = r;
		clipr(&screen, screen.r);
		d = balloc(r, 0);
		tv = localtime(time(0));
		if(tv->hour > 12)
			tv->hour -= 12;

		ha = 90 - ((360/12) * tv->hour) - ((360 * tv->min)/(60*12));
		ma = 90 - ((360/60) * tv->min);
		face(d, r, ha, ma);
		bitblt(&screen, r.min, d, r, S);
		bfree(d);
	}
}

void
face(Bitmap *d, Rectangle r, int ha, int ma)
{
	int xwidth, ywidth, radius, i;
	int sx, sy, siz;
	Point mid, p, top, up, dn;
	Bitmap *b;
	double inc;

	bitblt(d, r.min, d, r, F);

	xwidth = r.max.x - r.min.x;
	ywidth = r.max.y - r.min.y;

	mid.x = xwidth/2 + r.min.x;
	mid.y = ywidth/2 + r.min.y;
	
	radius = xwidth < ywidth ? xwidth : ywidth;
	radius = (radius/2) - 4;

	b = balloc(r, 0);
	circle(b, mid, radius, 1, F);
	bitblt(b, add(r.min, Pt(0, 1)), b, r, DorS);
	bitblt(b, add(r.min, Pt(1, 0)), b, r, DorS);
	if(radius > 100) {
		bitblt(b, add(r.min, Pt(0, -1)), b, r, DorS);
		bitblt(b, add(r.min, Pt(-1, 0)), b, r, DorS);
	}
	bitblt(d, r.min, b, r, notS);
	bfree(b);

	radius = (radius*18)/20;
	inc = -3.14159/2;
	for(i = 0; i < 12; i++) {
		sx = (int)(sin(inc) * (double)radius);
		sy = (int)(cos(inc) * (double)radius);
		p = add(mid, Pt(sx, sy));
		siz = 1;
		if(sx == 0 || sy == 0)
			siz += 1;
		disc(d, p, siz, 1, notS);
		inc += 3.141592/6;
	}

	radius = (radius*18)/20;
	top = limb(mid, ma, radius);
	up = limb(mid, ma-3, radius/2);
	dn = limb(mid, ma+3, radius/2);
	segment(d, mid, up, 1, notS);
	segment(d, mid, dn, 1, notS);
	segment(d, top, up, 1, notS);
	segment(d, top, dn, 1, notS);

	radius = (radius*2)/3;
	top = limb(mid, ha, radius);
	up = limb(mid, ha-6, radius/2);
	dn = limb(mid, ha+6, radius/2);
	segment(d, mid, up, 1, notS);
	segment(d, mid, dn, 1, notS);
	segment(d, top, up, 1, notS);
	segment(d, top, dn, 1, notS);

	bflush();
}

Point
limb(Point mid, int xa, int len)
{
	int sx, sy;
	double angle;

	angle = (3.14159*(double)xa)/180;;
	sx = (int)(cos(angle) * (double)len);
	sy = -(int)(sin(angle) * (double)len);
	return add(mid, Pt(sx, sy));
}
