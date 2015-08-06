/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum {
	Black		= 0x00,
	Blue		= 0x01,
	Green		= 0x02,
	Cyan		= 0x03,
	Red		= 0x04,
	Magenta		= 0x05,
	Brown		= 0x06,
	Grey		= 0x07,

	Bright 		= 0x08,
	Blinking	= 0x80,

	Attr		= (Black<<4)|Grey,	/* (background<<4)|foreground */
};

enum {
	Index		= 0x3d4,
	Data		= Index+1,

	Width		= 80*2,
	Height		= 25,

	Poststrlen	= 0,
	Postcodelen	= 2,
	Postlen		= Poststrlen+Postcodelen,

	Cgasize		= 16384,
};

#define CGA		(BIOSSEG(0xb800))

static Lock cgalock;
static int cgapos;
static int cgainitdone;

static int
cgaregr(int index)
{
	outb(Index, index);
	return inb(Data) & 0xff;
}

static void
cgaregw(int index, int data)
{
	outb(Index, index);
	outb(Data, data);
}

static void
cgablinkoff(void)
{
	cgaregw(0x0a, 1<<5);
}

static void
cgacursor(void)
{
	uint8_t *cga;

	cgaregw(0x0e, (cgapos/2>>8) & 0xff);
	cgaregw(0x0f, cgapos/2 & 0xff);

	cga = CGA;
	cga[cgapos+1] = Attr;
}

/*
 * extern, so we could use it to debug things like
 * lock() if necessary.
 */
void
cgaputc(int c)
{
	int i;
	uint8_t *cga, *p;

	cga = CGA;

	if(c == '\n'){
		cgapos = cgapos/Width;
		cgapos = (cgapos+1)*Width;
	}
	else if(c == '\t'){
		i = 8 - ((cgapos/2)&7);
		while(i-- > 0)
			cgaputc(' ');
	}
	else if(c == '\b'){
		if(cgapos >= 2)
			cgapos -= 2;
		cgaputc(' ');
		cgapos -= 2;
	}
	else{
		cga[cgapos++] = c;
		cga[cgapos++] = Attr;
	}
	if(cgapos >= (Width*Height)-Postlen*2){
		memmove(cga, &cga[Width], Width*(Height-1));
		p = &cga[Width*(Height-1)-Postlen*2];
		for(i = 0; i < Width/2; i++){
			*p++ = ' ';
			*p++ = Attr;
		}
		cgapos -= Width;
	}
	cgacursor();
}

int
cgaprint(int off, char *fmt, ...)
{
	va_list va;
	char buf[128];
	uint8_t *cga;
	int i, n;

	va_start(va, fmt);
	n = vsnprint(buf, sizeof buf, fmt, va);
	va_end(va);

	cga = CGA;
	for(i = 0; (2*(i+off))+1 < Cgasize && i < n; i++){
		cga[2*(i+off)+0] = buf[i];
		cga[2*(i+off)+1] = Attr;
	}
	return n;
}

int
cgaclearln(int off, int c)
{
	uint8_t  *cga;
	int i;

	cga = CGA;
	for(i = off; (2*i)+1 < Cgasize && i%80 != 0; i++){
		cga[2*i+0] = c;
		cga[2*i+1] = Attr;
	}
	return i-off;
}

/*
 * debug
 */
void
cgaprinthex(uintptr_t x)
{
	char str[30];
	char *s;
	static char dig[] = "0123456789abcdef";

	str[29] = 0;
	s = &str[29];
	while(x != 0){
		*--s = dig[x&0xF];
		x >>= 4;
	}
	while(*s != 0)
		cgaputc(*s++);
	cgaputc('\n');
}

void
cgaconsputs(char* s, int n)
{
	ilock(&(&cgalock)->lock);
	while(n-- > 0)
		cgaputc(*s++);
	iunlock(&(&cgalock)->lock);
}

void
cgapost(int code)
{
	uint8_t *cga;

	static char hex[] = "0123456789ABCDEF";

	cga = CGA;
	cga[Width*Height-Postcodelen*2] = hex[(code>>4) & 0x0f];
	cga[Width*Height-Postcodelen*2+1] = Attr;
	cga[Width*Height-Postcodelen*2+2] = hex[code & 0x0f];
	cga[Width*Height-Postcodelen*2+3] = Attr;
}

static int32_t
cgaread(Chan* c, void *vbuf, int32_t len, int64_t off)
{
	uint8_t *cga;
	extern int panicking;
	if(panicking)
		error("cgaread: kernel panic");
	if(off < 0 || off > Cgasize)
		error("cgaread: offset out of bounds");
	if(off+len > Cgasize)
		len = Cgasize - off;
	cga = CGA;
	memmove(vbuf, cga + off, len);
	return len;
}

static int32_t
cgawrite(Chan* c, void *vbuf, int32_t len, int64_t off)
{
	uint8_t *cga;
	extern int panicking;
	if(panicking)
		error("cgawrite: kernel panic");
	if(off < 0 || off > Cgasize)
		error("cgawrite: offset out of bounds");
	if(off+len > Cgasize)
		len = Cgasize - off;
	cga = CGA;
	memmove(cga + off, vbuf, len);
	return len;
}

void
cgainit(void)
{
	ilock(&(&cgalock)->lock);

	cgapos = cgaregr(0x0e)<<8;
	cgapos |= cgaregr(0x0f);
	cgapos *= 2;
	cgablinkoff();
	cgainitdone = 1;
	iunlock(&(&cgalock)->lock);
	addarchfile("cgamem", 0666, cgaread, cgawrite);
}
