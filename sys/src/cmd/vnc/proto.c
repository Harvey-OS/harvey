/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "vnc.h"

#define SHORT(p) (((p)[0] << 8) | ((p)[1]))
#define LONG(p) ((SHORT(p) << 16) | SHORT(p + 2))

uint8_t zero[64];

Vnc*
vncinit(int fd, int cfd, Vnc* v)
{
	if(v == nil)
		v = mallocz(sizeof(*v), 1);
	Binit(&v->in, fd, OREAD);
	Binit(&v->out, fd, OWRITE);
	v->datafd = fd;
	v->ctlfd = cfd;
	return v;
}

void
vncterm(Vnc* v)
{
	Bterm(&v->out);
	Bterm(&v->in);
}

void
vncflush(Vnc* v)
{
	if(Bflush(&v->out) < 0) {
		if(verbose > 1)
			fprint(2, "hungup while sending flush: %r\n");
		vnchungup(v);
	}
}

uint8_t
vncrdchar(Vnc* v)
{
	uint8_t buf[1];

	vncrdbytes(v, buf, 1);
	return buf[0];
}

uint16_t
vncrdshort(Vnc* v)
{
	uint8_t buf[2];

	vncrdbytes(v, buf, 2);
	return SHORT(buf);
}

uint32_t
vncrdlong(Vnc* v)
{
	uint8_t buf[4];

	vncrdbytes(v, buf, 4);
	return LONG(buf);
}

Point
vncrdpoint(Vnc* v)
{
	Point p;

	p.x = vncrdshort(v);
	p.y = vncrdshort(v);
	return p;
}

Rectangle
vncrdrect(Vnc* v)
{
	Rectangle r;

	r.min.x = vncrdshort(v);
	r.min.y = vncrdshort(v);
	r.max.x = r.min.x + vncrdshort(v);
	r.max.y = r.min.y + vncrdshort(v);
	return r;
}

Rectangle
vncrdcorect(Vnc* v)
{
	Rectangle r;

	r.min.x = vncrdchar(v);
	r.min.y = vncrdchar(v);
	r.max.x = r.min.x + vncrdchar(v);
	r.max.y = r.min.y + vncrdchar(v);
	return r;
}

void
vncrdbytes(Vnc* v, void* a, int n)
{
	if(Bread(&v->in, a, n) != n) {
		if(verbose > 1)
			fprint(2, "hungup while reading\n");
		vnchungup(v);
	}
}

Pixfmt
vncrdpixfmt(Vnc* v)
{
	Pixfmt fmt;
	uint8_t pad[3];

	fmt.bpp = vncrdchar(v);
	fmt.depth = vncrdchar(v);
	fmt.bigendian = vncrdchar(v);
	fmt.truecolor = vncrdchar(v);
	fmt.red.max = vncrdshort(v);
	fmt.green.max = vncrdshort(v);
	fmt.blue.max = vncrdshort(v);
	fmt.red.shift = vncrdchar(v);
	fmt.green.shift = vncrdchar(v);
	fmt.blue.shift = vncrdchar(v);
	vncrdbytes(v, pad, 3);
	return fmt;
}

char*
vncrdstring(Vnc* v)
{
	uint32_t len;
	char* s;

	len = vncrdlong(v);
	s = malloc(len + 1);
	assert(s != nil);

	vncrdbytes(v, s, len);
	s[len] = '\0';
	return s;
}

/*
 * on the server side of the negotiation protocol, we read
 * the client response and then run the negotiated function.
 * in some cases (e.g., TLS) the negotiated function needs to
 * use v->datafd directly and be sure that no data has been
 * buffered away in the Bio.  since we know the client is waiting
 * for our response, it won't have sent any until we respond.
 * thus we read the response with vncrdstringx, which goes
 * behind bio's back.
 */
char*
vncrdstringx(Vnc* v)
{
	char tmp[4];
	char* s;
	uint32_t len;

	assert(Bbuffered(&v->in) == 0);
	if(readn(v->datafd, tmp, 4) != 4) {
		fprint(2, "cannot rdstringx: %r");
		vnchungup(v);
	}
	len = LONG(tmp);
	s = malloc(len + 1);
	assert(s != nil);
	if(readn(v->datafd, s, len) != len) {
		fprint(2, "cannot rdstringx len %lud: %r", len);
		vnchungup(v);
	}
	s[len] = '\0';
	return s;
}

void
vncwrstring(Vnc* v, char* s)
{
	uint32_t len;

	len = strlen(s);
	vncwrlong(v, len);
	vncwrbytes(v, s, len);
}

void
vncwrbytes(Vnc* v, void* a, int n)
{
	if(Bwrite(&v->out, a, n) < 0) {
		if(verbose > 1)
			fprint(2, "hungup while writing bytes\n");
		vnchungup(v);
	}
}

void
vncwrlong(Vnc* v, uint32_t u)
{
	uint8_t buf[4];

	buf[0] = u >> 24;
	buf[1] = u >> 16;
	buf[2] = u >> 8;
	buf[3] = u;
	vncwrbytes(v, buf, 4);
}

void
vncwrshort(Vnc* v, uint16_t u)
{
	uint8_t buf[2];

	buf[0] = u >> 8;
	buf[1] = u;
	vncwrbytes(v, buf, 2);
}

void
vncwrchar(Vnc* v, uint8_t c)
{
	vncwrbytes(v, &c, 1);
}

void
vncwrpixfmt(Vnc* v, Pixfmt* fmt)
{
	vncwrchar(v, fmt->bpp);
	vncwrchar(v, fmt->depth);
	vncwrchar(v, fmt->bigendian);
	vncwrchar(v, fmt->truecolor);
	vncwrshort(v, fmt->red.max);
	vncwrshort(v, fmt->green.max);
	vncwrshort(v, fmt->blue.max);
	vncwrchar(v, fmt->red.shift);
	vncwrchar(v, fmt->green.shift);
	vncwrchar(v, fmt->blue.shift);
	vncwrbytes(v, zero, 3);
}

void
vncwrrect(Vnc* v, Rectangle r)
{
	vncwrshort(v, r.min.x);
	vncwrshort(v, r.min.y);
	vncwrshort(v, r.max.x - r.min.x);
	vncwrshort(v, r.max.y - r.min.y);
}

void
vncwrpoint(Vnc* v, Point p)
{
	vncwrshort(v, p.x);
	vncwrshort(v, p.y);
}

void
vnclock(Vnc* v)
{
	qlock(v);
}

void
vncunlock(Vnc* v)
{
	qunlock(v);
}

void
hexdump(void* a, int n)
{
	uint8_t* p, *ep;

	p = a;
	ep = p + n;

	for(; p < ep; p++)
		print("%.2ux ", *p);
	print("\n");
}

void
vncgobble(Vnc* v, int32_t n)
{
	uint8_t buf[8192];
	int32_t m;

	while(n > 0) {
		m = n;
		if(m > sizeof(buf))
			m = sizeof(buf);
		vncrdbytes(v, buf, m);
		n -= m;
	}
}
