#include <u.h>
#include <libc.h>
#include <draw.h>

int debug = 0;

struct Cal {
	double scalex;
	double transx;
	double scaley;
	double transy;
} cal;

/* Calibrate the pen */

Point pts[5];

Point pens[5];

Point up	= {  0, -10};
Point down	= {  0,  10};
Point left	= {-10,   0};
Point right	= { 10,   0};

Point
pen2scr(Point p)
{
	return Pt(p.x * cal.scalex / 65536.0 + cal.transx,
			  p.y * cal.scaley / 65536.0 + cal.transy);
}

Point
scr2pen(Point p)
{
	return Pt((p.x - cal.transx) * 65536.0 / cal.scalex,
			  (p.y + cal.transy) * 65536.0 / cal.scaley);
}

void
cross(Point p) {
	draw(screen, screen->r, display->white, nil, ZP);
	line(screen, addpt(p, up), addpt(p, down), Endsquare, Endsquare, 0, display->black, ZP);
	line(screen, addpt(p, left), addpt(p, right), Endsquare, Endsquare, 0, display->black, ZP);
	flushimage(display, 1);
}

Point
getmouse(int mouse) {
	static char buf[50];
	int x, y, b, n;
	Point p;

	n = 0;
	p.x = p.y = 0;
	do{
		if (read(mouse, buf, 48) != 48)
			sysfatal("read: %r");
		if (debug > 1) fprint(2, "%s\n", buf);
		if (buf[0] == 'r') {
			b = 1;
			continue;
		}
		if (buf[0] != 'm')
			sysfatal("mouse message: %s", buf);
		x = strtol(buf+1, nil, 10);
		y = strtol(buf+13, nil, 10);
		b = strtol(buf+25, nil, 10);
		p.x = (n * p.x + x)/(n+1);
		p.y = (n * p.y + y)/(n+1);
		n++;
	}while (b & 0x7);
	return p;
}

void
main(int argc, char **argv) {
	int i, mouse, mousectl, wctl, ntries;
	Rectangle r, oldsize;

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	}ARGEND;

	if((mouse = open("/dev/mouse", OREAD)) < 0
	 && (mouse = open("#m/mouse", OREAD)) < 0)
		sysfatal("#m/mouse: %r");

	mousectl = open("#m/mousectl", ORDWR);
	if(mousectl < 0)
		sysfatal("#m/mousectl: %r");

	if(initdraw(nil, nil, "calibrate") < 0)
		sysfatal("initdraw: %r");

	wctl = -1;
	for(ntries = 0; ntries < 3; ntries++){
		r = insetrect(display->image->r, 2);

		if(wctl < 0)
			wctl = open("/dev/wctl", ORDWR);
		if(wctl >= 0){
			char buf[4*12+1];

			buf[48] = 0;
			oldsize = screen->r;
			if(fprint(wctl, "resize -r %R", r) <= 0)
				sysfatal("write /dev/wctl, %r");
			if(getwindow(display, Refbackup) < 0)
				sysfatal("getwindow");
			if(debug) fprint(2, "resize: %R\n", screen->r);

			if(read(mousectl, buf, 48) != 48)
				sysfatal("read mousectl: %r");
			if(debug > 1) fprint(2, "mousectl %s\n", buf);
			if(buf[0] != 'c')
				sysfatal("mousectl message: %s", buf);
			cal.scalex = strtol(buf+ 1, nil, 10);
			cal.scaley = strtol(buf+13, nil, 10);
			cal.transx = strtol(buf+25, nil, 10);
			cal.transy = strtol(buf+37, nil, 10);
		}else{
			fprint(mousectl, "calibrate");
			cal.scalex = 1<<16;
			cal.scaley = 1<<16;
			cal.transx = 0;
			cal.transy = 0;
		}

		fprint(2, "calibrate %ld %ld %ld %ld (old)\n",
			(long)cal.scalex, (long)cal.scaley, (long)cal.transx, (long)cal.transy);

		if(debug)
			fprint(2, "screen coords %R\n", screen->r);
		pts[0] = addpt(Pt( 16,  16), screen->r.min);
		pts[1] = addpt(Pt( 16, -16), Pt(screen->r.min.x, screen->r.max.y));
		pts[2] = addpt(Pt(-16,  16), Pt(screen->r.max.x, screen->r.min.y));
		pts[3] = addpt(Pt(-16, -16), screen->r.max);
		pts[4] = Pt((screen->r.max.x + screen->r.min.x)/2,
					(screen->r.max.y + screen->r.min.y)/2);;

		for(i = 0; i < 4; i++){
			cross(pts[i]);
			pens[i] = scr2pen(getmouse(mouse));
			if (debug) fprint(2, "%P\n", pens[i]);
		}

		cal.scalex = (pts[2].x + pts[3].x - pts[0].x - pts[1].x) * 65536.0 /
				  (pens[2].x + pens[3].x - pens[0].x - pens[1].x);
		cal.scaley = (pts[1].y + pts[3].y - pts[0].y - pts[2].y) * 65536.0 /
				  (pens[1].y + pens[3].y - pens[0].y - pens[2].y);
		cal.transx = pts[0].x - (pens[0].x*cal.scalex) / 65536.0;
		cal.transy = pts[0].y - (pens[0].y*cal.scaley) / 65536.0;

		if(debug)
			fprint(2, "scale [%f, %f], trans [%f, %f]\n",
				cal.scalex, cal.scaley, cal.transx, cal.transy);

		fprint(mousectl, "calibrate %ld %ld %ld %ld",
			(long)cal.scalex, (long)cal.scaley, (long)cal.transx, (long)cal.transy);

		cross(pts[4]);
		pens[4] = getmouse(mouse);

		draw(screen, screen->r, display->white, nil, ZP);
		flushimage(display, 1);

		if(abs(pts[4].x-pens[4].x) <= 4 && abs(pts[4].y-pens[4].y) <= 4){
			fprint(2, "Calibration ok: %P -> %P\n", pts[4], pens[4]);
			break;
		}
		fprint(2, "Calibration inaccurate: %P -> %P\n", pts[4], pens[4]);
	}
	print("calibrate='%ld %ld %ld %ld'\n",
		(long)cal.scalex, (long)cal.scaley, (long)cal.transx, (long)cal.transy);

	if(debug)
		for(i = 0; i < 4; i++){
			cross(pts[i]);
			pens[i] = getmouse(mouse);
			if (debug) fprint(2, "%P\n", pens[i]);
		}

	if(wctl >= 0)
		fprint(wctl, "resize -r %R", oldsize);

}
