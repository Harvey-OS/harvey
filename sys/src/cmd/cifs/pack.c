/* packet packing and unpacking */
#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "cifs.h"

void *
pmem(Pkt *p, void *v, int len)
{
	uchar *str = v;
	void *s = p->pos;

	if(!len || !v)
		return s;
	while(len--)
		*p->pos++ = *str++;
	return s;
}

void *
ppath(Pkt *p, char *str)
{
	char c;
	Rune r;
	void *s = p->pos;

	if(!str)
		return s;

	if(p->s->caps & CAP_UNICODE){
		if(((p->pos - p->buf) % 2) != 0)	/* pad to even offset */
			p8(p, 0);
		while(*str){
			str += chartorune(&r, str);
			if(r == L'/')
				r = L'\\';
			pl16(p, r);
		}
		pl16(p, 0);
	} else {
		while((c = *str++) != 0){
			if(c == '/')
				c = '\\';
			*p->pos++ = c;
		}
		*p->pos++ = 0;
	}
	return s;
}

void *
pstr(Pkt *p, char *str)
{
	void *s = p->pos;
	Rune r;

	if(!str)
		return s;

	if(p->s->caps & CAP_UNICODE){
		if(((p->pos - p->buf) % 2) != 0)
			p8(p, 0);		/* pad to even offset */
		while(*str){
			str += chartorune(&r, str);
			pl16(p, r);
		}
		pl16(p, 0);
	} else {
		while(*str)
			*p->pos++ = *str++;
		*p->pos++ = 0;
	}
	return s;
}

void *
pascii(Pkt *p, char *str)
{
	void *s = p->pos;

	while(*str)
		*p->pos++ = *str++;
	*p->pos++ = 0;
	return s;
}


void *
pl64(Pkt *p, uvlong n)
{
	void *s = p->pos;

	*p->pos++ = n;
	*p->pos++ = n >> 8;
	*p->pos++ = n >> 16;
	*p->pos++ = n >> 24;
	*p->pos++ = n >> 32;
	*p->pos++ = n >> 40;
	*p->pos++ = n >> 48;
	*p->pos++ = n >> 56;
	return s;
}

void *
pb32(Pkt *p, uint n)
{
	void *s = p->pos;

	*p->pos++ = n >> 24;
	*p->pos++ = n >> 16;
	*p->pos++ = n >> 8;
	*p->pos++ = n;
	return s;
}

void *
pl32(Pkt *p, uint n)
{
	void *s = p->pos;

	*p->pos++ = n;
	*p->pos++ = n >> 8;
	*p->pos++ = n >> 16;
	*p->pos++ = n >> 24;
	return s;
}

void *
pb16(Pkt *p, uint n)
{
	void *s = p->pos;

	*p->pos++ = n >> 8;
	*p->pos++ = n;
	return s;
}

void *
pl16(Pkt *p, uint n)
{
	void *s = p->pos;

	*p->pos++ = n;
	*p->pos++ = n >> 8;
	return s;
}

void *
p8(Pkt *p, uint n)
{
	void *s = p->pos;

	*p->pos++ = n;
	return s;
}

/*
 * Encode a Netbios name
 */
void *
pname(Pkt *p, char *name, char pad)
{
	int i, done = 0;
	char c;
	void *s = p->pos;

	*p->pos++ = ' ';
	for(i = 0; i < 16; i++) {
		c = pad;
		if(!done && name[i] == '\0')
			done = 1;
		if(!done)
			c = islower(name[i])? toupper(name[i]): name[i];
		*p->pos++ = ((uchar)c >> 4) + 'A';
		*p->pos++ = (c & 0xf) + 'A';
	}
	*p->pos++ = '\0';
	return s;
}

void *
pvtime(Pkt *p, uvlong n)
{
	void *s = p->pos;

	n += 11644473600LL;
	n *= 10000000LL;

	pl32(p, n);
	pl32(p, n >> 32);
	return s;
}

void *
pdatetime(Pkt *p, long utc)
{
	void *s = p->pos;
	Tm *tm = localtime(utc);
	int t = tm->hour << 11 | tm->min << 5 | (tm->sec / 2);
	int d = (tm->year - 80) << 9 | (tm->mon + 1) << 5 | tm->mday;

	/*
	 * bug in word swapping in Win95 requires this
	 */
	if(p->s->caps & CAP_NT_SMBS){
		pl16(p, d);
		pl16(p, t);
	} else{
		pl16(p, t);
		pl16(p, d);
	}
	return s;
}


void
gmem(Pkt *p, void *v, int n)
{
	uchar *str = v;

	if(!n || !v)
		return;
	while(n-- && p->pos < p->eop)
		*str++ = *p->pos++;
}

/*
 * note len is the length of the source string in
 * in runes or bytes, in ASCII mode this is also the size
 * of the output buffer but this is not so in Unicode mode!
 */
void
gstr(Pkt *p, char *str, int n)
{
	int i;
	Rune r;

	if(!n || !str)
		return;

	if(p->s->caps & CAP_UNICODE){
		i = 0;
		while(*p->pos && n && p->pos < p->eop){
			r = gl16(p);
			i += runetochar(str +i, &r);
			n -= 2;
		}
		*(str + i) = 0;

		while(*p->pos && p->pos < p->eop)
			gl16(p);
		/*
		 * some versions of windows terminate a rune string
		 * with a single nul so we do a dangerous hack...
		 */
		if(p->pos[1])
			g8(p);
		else
			gl16(p);
	} else {
		while(*p->pos && n-- && p->pos < p->eop)
			*str++ = *p->pos++;
		*str = 0;
		while(*p->pos++ && p->pos < p->eop)
			continue;
	}
}

void
gascii(Pkt *p, char *str, int n)
{
	if(!n || !str)
		return;

	while(*p->pos && n-- && p->pos < p->eop)
		*str++ = *p->pos++;
	*str = 0;
	while(*p->pos++ && p->pos < p->eop)
		continue;
}


uvlong
gl64(Pkt *p)
{
	uvlong n;

	if(p->pos + 8 > p->eop)
		return 0;

	n  = (uvlong)*p->pos++;
	n |= (uvlong)*p->pos++ << 8;
	n |= (uvlong)*p->pos++ << 16;
	n |= (uvlong)*p->pos++ << 24;
	n |= (uvlong)*p->pos++ << 32;
	n |= (uvlong)*p->pos++ << 40;
	n |= (uvlong)*p->pos++ << 48;
	n |= (uvlong)*p->pos++ << 56;
	return n;
}

uvlong
gb48(Pkt *p)
{
	uvlong n;

	if(p->pos + 6 > p->eop)
		return 0;

	n  = (uvlong)*p->pos++ << 40;
	n |= (uvlong)*p->pos++ << 24;
	n |= (uvlong)*p->pos++ << 32;
	n |= (uvlong)*p->pos++ << 16;
	n |= (uvlong)*p->pos++ << 8;
	n |= (uvlong)*p->pos++;
	return n;
}

uint
gb32(Pkt *p)
{
	uint n;

	if(p->pos + 4 > p->eop)
		return 0;

	n  = (uint)*p->pos++ << 24;
	n |= (uint)*p->pos++ << 16;
	n |= (uint)*p->pos++ << 8;
	n |= (uint)*p->pos++;
	return n;
}

uint
gl32(Pkt *p)
{
	uint n;

	if(p->pos + 4 > p->eop)
		return 0;

	n  = (uint)*p->pos++;
	n |= (uint)*p->pos++ << 8;
	n |= (uint)*p->pos++ << 16;
	n |= (uint)*p->pos++ << 24;
	return n;
}

uint
gb16(Pkt *p)
{
	uint n;

	if(p->pos + 2 > p->eop)
		return 0;
	n  = (uint)*p->pos++ << 8;
	n |= (uint)*p->pos++;
	return n;
}

uint
gl16(Pkt *p)
{
	uint n;

	if(p->pos + 2 > p->eop)
		return 0;
	n  = (uint)*p->pos++;
	n |= (uint)*p->pos++ << 8;
	return n;
}

uint
g8(Pkt *p)
{
	if(p->pos + 1 > p->eop)
		return 0;
	return (uint)*p->pos++;
}

long
gdatetime(Pkt *p)
{
	Tm tm;
	uint d, t;

	if(p->pos + 4 > p->eop)
		return 0;

	/*
	 * bug in word swapping in Win95 requires this
	 */
	if(p->s->caps & CAP_NT_SMBS){
		d = gl16(p);
		t = gl16(p);
	}else{
		t = gl16(p);
		d = gl16(p);
	}

	tm.year = 80 + (d >> 9);
	tm.mon = ((d >> 5) & 017) - 1;
	tm.mday = d & 037;
	tm.zone[0] = 0;
	tm.tzoff = p->s->tz;

	tm.hour = t >> 11;
	tm.min = (t >> 5) & 63;
	tm.sec = (t & 31) << 1;

	return tm2sec(&tm);
}

long
gvtime(Pkt *p)
{
	uvlong vl;

	if(p->pos + 8 > p->eop)
		return 0;

	vl  = (uvlong)gl32(p);
	vl |= (uvlong)gl32(p) << 32;

	vl /= 10000000LL;
	vl -= 11644473600LL;
	return vl;
}

void
gconv(Pkt *p, int conv, char *str, int n)
{
	int off;
	uchar *pos;

	off = gl32(p) & 0xffff;
	if(off == 0 || p->tdata - conv + off > p->eop){
		memset(str, 0, n);
		return;
	}

	pos = p->pos;
	p->pos = p->tdata - conv + off;
	gascii(p, str, n);
	p->pos = pos;
}

void
goff(Pkt *p, uchar *base, char *str, int n)
{
	int off;
	uchar *pos;

	off = gl16(p);
	if(off == 0 || base + off > p->eop){
		memset(str, 0, n);
		return;
	}
	pos = p->pos;
	p->pos = base + off;
	gstr(p, str, n);
	p->pos = pos;
}
