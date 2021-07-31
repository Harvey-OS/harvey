#include "vnc.h"

Image *vncscreen;

static struct {
	char *name;
	int num;
} enctab[] = {
	"copyrect", rfbEncodingCopyRect,
	"corre", rfbEncodingCoRRE,
	"hextile", rfbEncodingHextile,
	"raw", rfbEncodingRaw,
	"rre", rfbEncodingRRE,
};

void
sendencodings(Vnc *v)
{
	char *f[10];
	int enc[10], nenc, i, j, nf;

	nf = tokenize(encodings, f, nelem(f));
	nenc = 0;
	for(i=0; i<nf; i++) {
		for(j=0; j<nelem(enctab); j++)
			if(strcmp(f[i], enctab[j].name) == 0)
				break;
		if(j == nelem(enctab)) {
			print("warning: unknown encoding %s\n", f[i]);
			continue;
		}
		enc[nenc++] = enctab[j].num;
	}

	Vwrchar(v, rfbMsgSetEncodings);
	Vwrchar(v, 0);
	Vwrshort(v, nenc);
	for(i=0; i<nenc; i++)
		Vwrlong(v, enc[i]);
	Vflush(v);
}

void
requestupdate(Vnc *v, int incremental)
{
	uchar buf[10];
	buf[0] = rfbMsgFramebufferUpdateRequest;
	buf[1] = incremental;
	buf[2] = buf[3] = 0;
	buf[4] = buf[5] = 0;
	buf[6] = v->dim.x>>8;
	buf[7] = v->dim.x;
	buf[8] = v->dim.y>>8;
	buf[9] = v->dim.y;
	write(Bfildes(&v->out), buf, 10);

//	Vlock(v);
//	Vwrchar(v, rfbMsgFramebufferUpdateRequest);
//	Vwrchar(v, incremental);
//	Vwrrect(v, Rpt(ZP, v->dim));
//	Vflush(v);
//	Vunlock(v);
}

void
updatescreen(Rectangle r)
{
	if(vncscreen != screen)
		draw(screen, rectaddpt(r, screen->r.min), vncscreen, nil, r.min);
}

static void
fillrect(Rectangle r, Color c)
{
	Image *im;

	if(cvtpixels)
		cvtpixels((uchar*)&c, 1);

	im = colorimage(c);
	r = rectaddpt(r, vncscreen->r.min);
	lockdisplay(display);
	draw(vncscreen, r, im, nil, r.min);
	updatescreen(r);
	unlockdisplay(display);
}

static void
Vgobble(Vnc *v, long n)
{
	uchar buf[8192];
	long m;

	while(n > 0) {
		m = n;
		if(m > sizeof(buf))
			m = sizeof(buf);
		Vrdbytes(v, buf, m);
		n -= m;
	}
}

static uchar rawbuf[640*480];

static void
loadbuf(uchar *buf, Rectangle r)
{
	long n, m;

	m = Dy(r) * Dx(r) * display->depth/8;
	r = rectaddpt(r, vncscreen->r.min);
	lockdisplay(display);
	if((n=loadimage(vncscreen, r, buf, m)) != m)
		print("loadimage(%R, %lud) in %R = %ld: %r\n", r, m, vncscreen->r, n);
	updatescreen(r);
	unlockdisplay(display);
}

static Rectangle
hexrect(ushort u)
{
	int x, y, w, h;

	x = u>>12;
	y = (u>>8)&15;
	w = ((u>>4)&15)+1;
	h = (u&15)+1;

	return Rect(x, y, x+w, y+h);
}


static void
dohextile(Vnc *v, Rectangle r)
{
	Color bg, fg, c;
	int enc, nsub, x, y;
	Rectangle sr, ssr;

	fg = bg = 0;
	for(y=r.min.y; y<r.max.y; y+=16) {
		for(x=r.min.x; x<r.max.x; x+=16) {
			sr = Rect(x, y, x+16, y+16);
			rectclip(&sr, r);

			enc = Vrdchar(v);
			if(enc & rfbHextileRaw) {
				Vrdbytes(v, rawbuf, Dx(sr)*Dy(sr)*v->bpp/8);
				if(cvtpixels)
					cvtpixels(rawbuf, Dx(sr)*Dy(sr));
				loadbuf(rawbuf, sr);
				continue;
			}

			if(enc & rfbHextileBackground)
				bg = Vrdcolor(v);
			fillrect(sr, bg);

			if(enc & rfbHextileForeground)
				fg = Vrdcolor(v);

			if(enc & rfbHextileAnySubrects) {
				nsub = Vrdchar(v);
				c = fg;
				while(nsub-- > 0) {
					if(enc & rfbHextileSubrectsColored)
						c = Vrdcolor(v);
					ssr = rectaddpt(hexrect(Vrdshort(v)), sr.min);
					fillrect(ssr, c);
				}
			}
		}
	}
}

static void
dorectangle(Vnc *v)
{
	ulong vbpl, dy, type;
	long n;
	Color c;
	Point p;
	Rectangle r, subr;

	r = Vrdrect(v);
	type = Vrdlong(v);
	switch(type) {
	case rfbEncodingRaw:
		vbpl = Dx(r) * v->bpp/8;
		dy = sizeof(rawbuf)/vbpl;
		subr = r;
		while(Dy(subr) > 0) {
			if(dy > Dy(subr))
				dy = Dy(subr);

			Vrdbytes(v, rawbuf, vbpl*dy);
			r = Rect(subr.min.x, subr.min.y, subr.max.x, subr.min.y+dy);
			if(cvtpixels)
				cvtpixels(rawbuf, Dx(r)*Dy(r));
			loadbuf(rawbuf, r);
			subr.min.y += dy;
		}
		break;

	case rfbEncodingCopyRect:
		p = addpt(Vrdpoint(v), vncscreen->r.min);
		r = rectaddpt(r, vncscreen->r.min);

		lockdisplay(display);
		draw(vncscreen, r, vncscreen, nil, p);
		updatescreen(r);
		unlockdisplay(display);
		break;

	case rfbEncodingRRE:
		n = Vrdlong(v);
		fillrect(r, Vrdcolor(v));
		while(n-- > 0) {
			c = Vrdcolor(v);
			subr = rectaddpt(Vrdrect(v), r.min);
			fillrect(subr, c);
		}
		break;

	case rfbEncodingCoRRE:
		n = Vrdlong(v);
		fillrect(r, Vrdcolor(v));
		while(n-- > 0) {
			c = Vrdcolor(v);
			subr = rectaddpt(Vrdcorect(v), r.min);
			fillrect(subr, c);
		}
		break;

	case rfbEncodingHextile:
		dohextile(v, r);
		break;
	}
}

void
readfromserver(Vnc *v)
{
	uchar type;
	uchar junk[100];
	long n;

	for(;;) {
		type = Vrdchar(v);
		switch(type) {
		case rfbMsgFramebufferUpdate:
			Vrdchar(v);
			n = Vrdshort(v);
			while(n-- > 0)
				dorectangle(v);
			flushimage(display, 1);
			requestupdate(v, 1);
			break;

		case rfbMsgSetColorMapEntries:
			Vrdbytes(v, junk, 3);
			n = Vrdshort(v);
			Vgobble(v, n*3*2);
			break;

		case rfbMsgBell:
			break;

		case rfbMsgServerCutText:
			Vrdbytes(v, junk, 3);
			n = Vrdlong(v);
			Vgobble(v, n);
			break;
		}
	}
}
