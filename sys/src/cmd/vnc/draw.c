#include "vnc.h"
#include "vncv.h"

static struct {
	char	*name;
	int	num;
} enctab[] = {
	"copyrect",	EncCopyRect,
	"corre",	EncCorre,
	"hextile",	EncHextile,
	"raw",		EncRaw,
	"rre",		EncRre,
	"mousewarp",	EncMouseWarp,
};

static	uchar	*pixbuf;
static	uchar	*linebuf;
static	int	vpixb;
static	int	pixb;
static	void	(*pixcp)(uchar*, uchar*);

static void
vncrdcolor(Vnc *v, uchar *color)
{
	vncrdbytes(v, color, vpixb);

	if(cvtpixels)
		(*cvtpixels)(color, color, 1);
}

void
sendencodings(Vnc *v)
{
	char *f[10];
	int enc[10], nenc, i, j, nf;

	nf = tokenize(encodings, f, nelem(f));
	nenc = 0;
	for(i=0; i<nf; i++){
		for(j=0; j<nelem(enctab); j++)
			if(strcmp(f[i], enctab[j].name) == 0)
				break;
		if(j == nelem(enctab)){
			print("warning: unknown encoding %s\n", f[i]);
			continue;
		}
		enc[nenc++] = enctab[j].num;
	}

	vnclock(v);
	vncwrchar(v, MSetEnc);
	vncwrchar(v, 0);
	vncwrshort(v, nenc);
	for(i=0; i<nenc; i++)
		vncwrlong(v, enc[i]);
	vncflush(v);
	vncunlock(v);
}

void
requestupdate(Vnc *v, int incremental)
{
	int x, y;

	lockdisplay(display);
	x = Dx(screen->r);
	y = Dy(screen->r);
	unlockdisplay(display);
	if(x > v->dim.x)
		x = v->dim.x;
	if(y > v->dim.y)
		y = v->dim.y;
	vnclock(v);
	vncwrchar(v, MFrameReq);
	vncwrchar(v, incremental);
	vncwrrect(v, Rpt(ZP, Pt(x, y)));
	vncflush(v);
	vncunlock(v);
}

static Rectangle
clippixbuf(Rectangle r, int maxx, int maxy)
{
	int y, h, stride1, stride2;

	if(r.min.x > maxx || r.min.y > maxy){
		r.max.x = 0;
		return r;
	}
	if(r.max.y > maxy)
		r.max.y = maxy;
	if(r.max.x <= maxx)
		return r;

	stride2 = Dx(r) * pixb;
	r.max.x = maxx;
	stride1 = Dx(r) * pixb;
	h = Dy(r);
	for(y = 0; y < h; y++)
		memmove(&pixbuf[y * stride1], &pixbuf[y * stride2], stride1);

	return r;
}

/* must be called with display locked */
static void
updatescreen(Rectangle r)
{
	int b, bb;

	lockdisplay(display);
	if(r.max.x > Dx(screen->r) || r.max.y > Dy(screen->r)){
		r = clippixbuf(r, Dx(screen->r), Dy(screen->r));
		if(r.max.x == 0){
			unlockdisplay(display);
			return;
		}
	}

	/*
	 * assume load image fails only because of resize
	 */
	b = Dx(r) * pixb * Dy(r);
	bb = loadimage(screen, rectaddpt(r, screen->r.min), pixbuf, b);
	if(bb != b && verbose)
		fprint(2, "loadimage %d on %R for %R returned %d: %r\n", b, rectaddpt(r, screen->r.min), screen->r, bb);
	unlockdisplay(display);
}

static void
fillrect(Rectangle r, int stride, uchar *color)
{
	int x, xe, y, off;

	y = r.min.y;
	off = y * stride;
	for(; y < r.max.y; y++){
		xe = off + r.max.x * pixb;
		for(x = off + r.min.x * pixb; x < xe; x += pixb)
			(*pixcp)(&pixbuf[x], color);
		off += stride;
	}
}

static void
loadbuf(Vnc *v, Rectangle r, int stride)
{
	int off, y;

	if(cvtpixels){
		y = r.min.y;
		off = y * stride;
		for(; y < r.max.y; y++){
			vncrdbytes(v, linebuf, Dx(r) * vpixb);
			(*cvtpixels)(&pixbuf[off + r.min.x * pixb], linebuf, Dx(r));
			off += stride;
		}
	}else{
		y = r.min.y;
		off = y * stride;
		for(; y < r.max.y; y++){
			vncrdbytes(v, &pixbuf[off + r.min.x * pixb], Dx(r) * pixb);
			off += stride;
		}
	}
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
dohextile(Vnc *v, Rectangle r, int stride)
{
	ulong bg, fg, c;
	int enc, nsub, sx, sy, w, h, th, tw;
	Rectangle sr, ssr;

	fg = bg = 0;
	h = Dy(r);
	w = Dx(r);
	for(sy = 0; sy < h; sy += HextileDim){
		th = h - sy;
		if(th > HextileDim)
			th = HextileDim;
		for(sx = 0; sx < w; sx += HextileDim){
			tw = w - sx;
			if(tw > HextileDim)
				tw = HextileDim;

			sr = Rect(sx, sy, sx + tw, sy + th);
			enc = vncrdchar(v);
			if(enc & HextileRaw){
				loadbuf(v, sr, stride);
				continue;
			}

			if(enc & HextileBack)
				vncrdcolor(v, (uchar*)&bg);
			fillrect(sr, stride, (uchar*)&bg);

			if(enc & HextileFore)
				vncrdcolor(v, (uchar*)&fg);

			if(enc & HextileRects){
				nsub = vncrdchar(v);
				(*pixcp)((uchar*)&c, (uchar*)&fg);
				while(nsub-- > 0){
					if(enc & HextileCols)
						vncrdcolor(v, (uchar*)&c);
					ssr = rectaddpt(hexrect(vncrdshort(v)), sr.min);
					fillrect(ssr, stride, (uchar*)&c);
				}
			}
		}
	}
}

static void
dorectangle(Vnc *v)
{
	ulong type;
	long n, stride;
	ulong color;
	Point p;
	Rectangle r, subr, maxr;

	r = vncrdrect(v);
	if(r.min.x == r.max.x || r.min.y == r.max.y)
		return;
	if(!rectinrect(r, Rpt(ZP, v->dim)))
		sysfatal("bad rectangle from server: %R not in %R", r, Rpt(ZP, v->dim));
	stride = Dx(r) * pixb;
	type = vncrdlong(v);
	switch(type){
	default:
		sysfatal("bad rectangle encoding from server");
		break;
	case EncRaw:
		loadbuf(v, Rpt(ZP, Pt(Dx(r), Dy(r))), stride);
		updatescreen(r);
		break;

	case EncCopyRect:
		p = vncrdpoint(v);
		lockdisplay(display);
		p = addpt(p, screen->r.min);
		r = rectaddpt(r, screen->r.min);
		draw(screen, r, screen, nil, p);
		unlockdisplay(display);
		break;

	case EncRre:
	case EncCorre:
		maxr = Rpt(ZP, Pt(Dx(r), Dy(r)));
		n = vncrdlong(v);
		vncrdcolor(v, (uchar*)&color);
		fillrect(maxr, stride, (uchar*)&color);
		while(n-- > 0){
			vncrdcolor(v, (uchar*)&color);
			if(type == EncRre)
				subr = vncrdrect(v);
			else
				subr = vncrdcorect(v);
			if(!rectinrect(subr, maxr))
				sysfatal("bad encoding from server");
			fillrect(subr, stride, (uchar*)&color);
		}
		updatescreen(r);
		break;

	case EncHextile:
		dohextile(v, r, stride);
		updatescreen(r);
		break;

	case EncMouseWarp:
		mousewarp(r.min);
		break;
	}
}

static void
pixcp8(uchar *dst, uchar *src)
{
	*dst = *src;
}

static void
pixcp16(uchar *dst, uchar *src)
{
	*(ushort*)dst = *(ushort*)src;
}

static void
pixcp32(uchar *dst, uchar *src)
{
	*(ulong*)dst = *(ulong*)src;
}

static void
pixcp24(uchar *dst, uchar *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}

static int
calcpixb(int bpp)
{
	if(bpp / 8 * 8 != bpp)
		sysfatal("can't handle your screen");
	return bpp / 8;
}

void
readfromserver(Vnc *v)
{
	uchar type;
	uchar junk[100];
	long n;

	vpixb = calcpixb(v->bpp);
	pixb = calcpixb(screen->depth);
	switch(pixb){
	case 1:
		pixcp = pixcp8;
		break;
	case 2:
		pixcp = pixcp16;
		break;
	case 3:
		pixcp = pixcp24;
		break;
	case 4:
		pixcp = pixcp32;
		break;
	default:
		sysfatal("can't handle your screen: bad depth %d", pixb);
	}
	linebuf = malloc(v->dim.x * vpixb);
	pixbuf = malloc(v->dim.x * pixb * v->dim.y);
	if(linebuf == nil || pixbuf == nil)
		sysfatal("can't allocate pix decompression storage");
	for(;;){
		type = vncrdchar(v);
		switch(type){
		default:
			sysfatal("bad message from server");
			break;
		case MFrameUpdate:
			vncrdchar(v);
			n = vncrdshort(v);
			while(n-- > 0)
				dorectangle(v);
			flushimage(display, 1);
			requestupdate(v, 1);
			break;

		case MSetCmap:
			vncrdbytes(v, junk, 3);
			n = vncrdshort(v);
			vncgobble(v, n*3*2);
			break;

		case MBell:
			break;

		case MSAck:
			break;

		case MSCut:
			vncrdbytes(v, junk, 3);
			n = vncrdlong(v);
			writesnarf(v, n);
			break;
		}
	}
}
