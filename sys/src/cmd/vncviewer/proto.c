#include "vnc.h"

#define SHORT(p) (((p)[0]<<8)|((p)[1]))
#define LONG(p) ((SHORT(p)<<16)|SHORT(p+2))

uchar zero[64];

Vnc*
openvnc(int fd)
{
	Vnc *v;

	v = mallocz(sizeof(*v), 1);
	Binit(&v->in, fd, OREAD);
	Binit(&v->out, fd, OWRITE);
	return v;
}

void
Verror(char *fmt, ...)
{
	char buf[1024];
	va_list va;

	va_start(va, fmt);
	doprint(buf, buf+sizeof buf, fmt, va);
	va_end(va);
	fprint(2, "%s: error talking with server: %s\n", argv0, buf);
	sysfatal(buf);
}

void
Vflush(Vnc *v)
{
	if(Bflush(&v->out) < 0)
		Verror("flush: %r");
}

static void
eBread(Biobuf *b, void *a, int n, char *what)
{
	if(Bread(b, a, n) != n)
		Verror("short read reading %s", what);
}

uchar
Vrdchar(Vnc *v)
{
	uchar buf[1];

	eBread(&v->in, buf, sizeof buf, "char");
	return buf[0];
}

ushort
Vrdshort(Vnc *v)
{
	uchar buf[2];

	eBread(&v->in, buf, sizeof buf, "short");
	return SHORT(buf);
}

ulong
Vrdlong(Vnc *v)
{
	uchar buf[4];

	eBread(&v->in, buf, sizeof buf, "long");
	return LONG(buf);
}

Point
Vrdpoint(Vnc *v)
{
	Point p;

	p.x = Vrdshort(v);
	p.y = Vrdshort(v);
	return p;
}

Rectangle
Vrdrect(Vnc *v)
{
	Rectangle r;

	r.min.x = Vrdshort(v);
	r.min.y = Vrdshort(v);
	r.max.x = r.min.x + Vrdshort(v);
	r.max.y = r.min.y + Vrdshort(v);
	return r;
}

Rectangle
Vrdcorect(Vnc *v)
{
	Rectangle r;

	r.min.x = Vrdchar(v);
	r.min.y = Vrdchar(v);
	r.max.x = r.min.x + Vrdchar(v);
	r.max.y = r.min.y + Vrdchar(v);
	return r;
}

void
Vrdbytes(Vnc *v, void *a, int n)
{
	Vflush(v);
	eBread(&v->in, a, n, "rdbytes");
}

Color
Vrdcolor(Vnc *v)
{
	Color c;

	c = 0;
	Vrdbytes(v, (uchar*)&c, v->bpp/8);
	return c;
}

Pixfmt
Vrdpixfmt(Vnc *v)
{
	Pixfmt fmt;
	uchar pad[3];

	fmt.bpp = Vrdchar(v);
	fmt.depth = Vrdchar(v);
	fmt.bigendian = Vrdchar(v);
	fmt.truecolor = Vrdchar(v);
	fmt.red.max = Vrdshort(v);
	fmt.green.max = Vrdshort(v);
	fmt.blue.max = Vrdshort(v);
	fmt.red.shift = Vrdchar(v);
	fmt.green.shift = Vrdchar(v);
	fmt.blue.shift = Vrdchar(v);
	eBread(&v->in, pad, 3, "padding");
	return fmt;
}

char*
Vrdstring(Vnc *v)
{
	ulong len;
	char *s;

	len = Vrdlong(v);
	s = malloc(len+1);
	assert(s != nil);

	Vrdbytes(v, s, len);
	s[len] = '\0';
	return s;
}

void
Vwrbytes(Vnc *v, void *a, int n)
{
	if(Bwrite(&v->out, a, n) < 0)
		Verror("wrbytes: %r");
}

void
Vwrlong(Vnc *v, ulong u)
{
	uchar buf[4];

	buf[0] = u>>24;
	buf[1] = u>>16;
	buf[2] = u>>8;
	buf[3] = u;
	Vwrbytes(v, buf, 4);
}

void
Vwrshort(Vnc *v, ushort u)
{
	uchar buf[2];

	buf[0] = u>>8;
	buf[1] = u;
	Vwrbytes(v, buf, 2);
}

void
Vwrchar(Vnc *v, uchar c)
{
	Vwrbytes(v, &c, 1);
}

void
Vwrpixfmt(Vnc *v, Pixfmt *fmt)
{
	Vwrchar(v, fmt->bpp);
	Vwrchar(v, fmt->depth);
	Vwrchar(v, fmt->bigendian);
	Vwrchar(v, fmt->truecolor);
	Vwrshort(v, fmt->red.max);
	Vwrshort(v, fmt->green.max);
	Vwrshort(v, fmt->blue.max);
	Vwrchar(v, fmt->red.shift);
	Vwrchar(v, fmt->green.shift);
	Vwrchar(v, fmt->blue.shift);
	Vwrbytes(v, zero, 3);
}

void
Vwrrect(Vnc *v, Rectangle r)
{
	Vwrshort(v, r.min.x);
	Vwrshort(v, r.min.y);
	Vwrshort(v, r.max.x);
	Vwrshort(v, r.max.y);
}

void
Vwrpoint(Vnc *v, Point p)
{
	Vwrshort(v, p.x);
	Vwrshort(v, p.y);
}

void
Vlock(Vnc *v)
{
	qlock(v);
}

void
Vunlock(Vnc *v)
{
	qunlock(v);
}

void
hexdump(void *a, int n)
{
	uchar *p, *ep;

	p = a;
	ep = p+n;

	for(; p<ep; p++) 
		print("%.2ux ", *p);
	print("\n");
}
