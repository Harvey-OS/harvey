#include "vnc.h"
#include <cursor.h>

typedef struct Cursor Cursor;

static Rectangle
getscreenrect(void)
{
	char buf[5*12];
	int fd;
	Rectangle r;

	fd = open("/dev/screen", OREAD);
	if(fd < 0)
		sysfatal("open /dev/screen: %r");
	if(read(fd, buf, sizeof buf) != sizeof buf)
		sysfatal("read /dev/screen: %r");
	close(fd);

	r.min.x = atoi(buf+1*12);
	r.min.y = atoi(buf+2*12);
	r.max.x = atoi(buf+3*12);
	r.max.y = atoi(buf+4*12);
	return r;
}

static Rectangle physr;

static void
resize(Vnc *v)
{
	int fd;
	Point d;
	Rectangle r;

	d = addpt(v->dim, Pt(2*Borderwidth, 2*Borderwidth));

	if(d.x > Dx(physr) || d.y > Dy(physr)) {
		vncscreen = allocimage(display, Rpt(ZP, d), display->chan, 0, DNofill);
		r = physr;
	} else {
		vncscreen = screen;
		r = Rpt(screen->r.min, addpt(screen->r.min, d));
		if(r.max.x > physr.max.x)
			screen->r.min.x = screen->r.max.x - d.x;
		if(r.max.y > physr.max.y)
			screen->r.min.y = screen->r.max.y - d.y;
	}

	fd = open("/dev/wctl", OWRITE);
	if(fd < 0)
		sysfatal("open /dev/wctl: %r");

	fprint(fd, "resize -r %d %d %d %d", r.min.x, r.min.y, r.max.x, r.max.y);
	close(fd);
	lockdisplay(display);
	getwindow(display, Refnone);
	unlockdisplay(display);
}

void
initwindow(Vnc *v)
{
	physr = getscreenrect();
	resize(v);
}

static void
eresized(int new)
{
	lockdisplay(display);
	if(new && getwindow(display, Refnone) < 0)
		fprint(2, "eresized: can't reattach to window");
	unlockdisplay(display);

	if(Dx(screen->r) != vnc->dim.x || Dy(screen->r) != vnc->dim.y)
		resize(vnc);

	requestupdate(vnc, 0);
}

Cursor crosscursor = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, },
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, }
};

Cursor dotcursor = {
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x03, 0xc0, 0x07, 0xe0, 0x07, 0xe0, 
	 0x07, 0xe0, 0x07, 0xe0, 0x03, 0xc0, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x07, 0xc0, 
	 0x07, 0xc0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

static void
mouseevent(Vnc *v, Mouse m)
{
	uchar buf[6];

	buf[0] = rfbMsgPointerEvent;
	buf[1] = m.buttons;
	buf[2] = m.xy.x >> 8;
	buf[3] = m.xy.x;
	buf[4] = m.xy.y >> 8;
	buf[5] = m.xy.y;
	write(Bfildes(&v->out), buf, 6);

//	Vlock(v);
//	Vwrchar(v, rfbMsgPointerEvent);
//	Vwrchar(v, m.buttons);
//	Vwrpoint(v, m.xy);
//	Vflush(v);
//	Vunlock(v);
}

enum {
	EventSize = 1+4*12
};
void
readmouse(Vnc *v)
{
	int cursorfd, fd, len, n;
	char buf[10*EventSize], *start, *end;
	uchar curs[2*4+2*2*16];
	Cursor *cs;
	Mouse m;

	cs = &dotcursor;
	eresized(0);

	snprint(buf, sizeof buf, "%s/mouse", display->devdir);
	if((fd = open(buf, OREAD)) < 0)
		sysfatal("open %s: %r", buf);

	snprint(buf, sizeof buf, "%s/cursor", display->devdir);
	if((cursorfd = open(buf, OWRITE)) < 0)
		sysfatal("open %s: %r", buf);

	BPLONG(curs+0*4, cs->offset.x);
	BPLONG(curs+1*4, cs->offset.y);
	memmove(curs+2*4, cs->clr, 2*2*16);
	write(cursorfd, curs, sizeof curs);

	start = end = buf;
	len = 0;
	for(;;) {
		if((n = read(fd, end, sizeof(buf) - (end - buf))) < 0)
			sysfatal("read mouse failed");

		len += n;
		end += n;
		while(len >= EventSize) {
			if(*start == 'm') {
				m.xy.x = atoi(start+1);
				m.xy.y = atoi(start+1+12);
				m.buttons = atoi(start+1+2*12) & 7;
				m.xy = subpt(m.xy, screen->r.min);
				if(ptinrect(m.xy, Rpt(ZP, v->dim)))
					mouseevent(v, m);
			} else
				eresized(1);

			start += EventSize;
			len -= EventSize;
		}
		if(start - buf > sizeof(buf) - EventSize) {
			memmove(buf, start, len);
			start = buf;
			end = start+len;
		}
	}
}
