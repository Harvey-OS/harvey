/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int
namecmp(char *s, char *t)
{
	return strncmp(s, t, ANameSize);
}

void
namecp(char *dst, char *src)
{
	strncpy(dst, src, ANameSize - 1);
	dst[ANameSize - 1] = '\0';
}

int
nameok(char *name)
{
	char *t;
	int c;

	if(name == nil)
		return -1;
	for(t = name; c = *t; t++)
		if(t - name >= ANameSize
		|| c < ' ' || c >= 0x7f)
			return -1;
	return 0;
}

int
stru32int(char *s, uint32_t *r)
{
	char *t;
	uint32_t n, nn, m;
	int c;

	m = TWID32 / 10;
	n = 0;
	for(t = s; ; t++){
		c = *t;
		if(c < '0' || c > '9')
			break;
		if(n > m)
			return -1;
		nn = n * 10 + c - '0';
		if(nn < n)
			return -1;
		n = nn;
	}
	*r = n;
	return s != t && *t == '\0';
}

int
stru64int(char *s, uint64_t *r)
{
	char *t;
	uint64_t n, nn, m;
	int c;

	m = TWID64 / 10;
	n = 0;
	for(t = s; ; t++){
		c = *t;
		if(c < '0' || c > '9')
			break;
		if(n > m)
			return -1;
		nn = n * 10 + c - '0';
		if(nn < n)
			return -1;
		n = nn;
	}
	*r = n;
	return s != t && *t == '\0';
}

int
vttypevalid(int type)
{
	return type < VtMaxType;
}

static char*
logit(int severity, char *fmt, va_list args)
{
	char *s;

	s = vsmprint(fmt, args);
	if(s == nil)
		return nil;
	if(severity != EOk){
		if(argv0 == nil)
			fprint(2, "%T %s: err %d: %s\n", argv0, severity, s);
		else
			fprint(2, "%T err %d: %s\n", severity, s);
	}
	return s;
}

void
seterr(int severity, char *fmt, ...)
{
	char *s;
	va_list args;

	va_start(args, fmt);
	s = logit(severity, fmt, args);
	va_end(args);
	if(s == nil)
		werrstr("error setting error");
	else{
		werrstr("%s", s);
		free(s);
	}
}

void
logerr(int severity, char *fmt, ...)
{
	char *s;
	va_list args;

	va_start(args, fmt);
	s = logit(severity, fmt, args);
	va_end(args);
	free(s);
}

uint32_t
now(void)
{
	return time(nil);
}

int abortonmem = 1;

void *
emalloc(uint32_t n)
{
	void *p;

	p = malloc(n);
	if(p == nil){
		if(abortonmem)
			abort();
		sysfatal("out of memory allocating %lud", n);
	}
	memset(p, 0xa5, n);
	setmalloctag(p, getcallerpc());
if(0)print("emalloc %p-%p by %#p\n", p, (char*)p+n, getcallerpc());
	return p;
}

void *
ezmalloc(uint32_t n)
{
	void *p;

	p = malloc(n);
	if(p == nil){
		if(abortonmem)
			abort();
		sysfatal("out of memory allocating %lud", n);
	}
	memset(p, 0, n);
	setmalloctag(p, getcallerpc());
if(0)print("ezmalloc %p-%p by %#p\n", p, (char*)p+n, getcallerpc());
	return p;
}

void *
erealloc(void *p, uint32_t n)
{
	p = realloc(p, n);
	if(p == nil){
		if(abortonmem)
			abort();
		sysfatal("out of memory allocating %lud", n);
	}
	setrealloctag(p, getcallerpc());
if(0)print("erealloc %p-%p by %#p\n", p, (char*)p+n, getcallerpc());
	return p;
}

char *
estrdup(char *s)
{
	char *t;
	int n;

	n = strlen(s) + 1;
	t = emalloc(n);
	memmove(t, s, n);
	setmalloctag(t, getcallerpc());
if(0)print("estrdup %p-%p by %#p\n", t, (char*)t+n, getcallerpc());
	return t;
}

/*
 * return floor(log2(v))
 */
int
u64log2(uint64_t v)
{
	int i;

	for(i = 0; i < 64; i++)
		if((v >> i) <= 1)
			break;
	return i;
}

int
vtproc(void (*fn)(void*), void *arg)
{
	proccreate(fn, arg, 256*1024);
	return 0;
}

int
ientryfmt(Fmt *fmt)
{
	IEntry *ie;

	ie = va_arg(fmt->args, IEntry*);
	return fmtprint(fmt, "%V %22lld %3d %5d %3d",
		ie->score, ie->ia.addr, ie->ia.type, ie->ia.size, ie->ia.blocks);
}

void
ventifmtinstall(void)
{
	fmtinstall('F', vtfcallfmt);
	fmtinstall('H', encodefmt);
	fmtinstall('I', ientryfmt);
	fmtinstall('T', vttimefmt);
	fmtinstall('V', vtscorefmt);
}

uint
msec(void)
{
	return nsec()/1000000;
}

uint
countbits(uint n)
{
	n = (n&0x55555555)+((n>>1)&0x55555555);
	n = (n&0x33333333)+((n>>2)&0x33333333);
	n = (n&0x0F0F0F0F)+((n>>4)&0x0F0F0F0F);
	n = (n&0x00FF00FF)+((n>>8)&0x00FF00FF);
	n = (n&0x0000FFFF)+((n>>16)&0x0000FFFF);
	return n;
}
