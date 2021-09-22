/*
 * color graphics adapter - use as dumb text console
 *
 * there are two bytes in cga memory for each byte displayed:
 * the character and its attributes (e.g. color).
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

enum {
	Black,
	Blue,
	Green,
	Cyan,
	Red,
	Magenta,
	Brown,
	Grey,

	Bright = 0x08,
	Blinking = 0x80,

	Yellow = Bright|Brown,
	White = Bright|Grey,

	Pindex	= 0x3d4,			/* motorola 6845 i/o ports */
	Pdata,
};

enum {
	Width		= 80*2,
	Height		= 25,

			/* high nibble background, low foreground */
	Attr		= (Black<<4)|Grey,

	Postcodewid	= 2,			/* in characters */

	Pcode		= 0x80,			/* post-code display port */
};

#define CGA	((uchar*)CGAMEM)		/* screen base */

static unsigned cgapos;
static Lock cgascreenlock;

/* access 6845 registers via i/o ports */
static uchar
cgaregr(uchar index)
{
	outb(Pindex, index);
	return inb(Pdata) & 0xFF;
}

static void
cgaregw(uchar index, uchar data)
{
	outb(Pindex, index);
	outb(Pdata, data);
}

static void
movecursor(void)
{
	cgaregw(0x0E, (cgapos/2>>8) & 0xFF);
	cgaregw(0x0F, cgapos/2 & 0xFF);
	CGA[cgapos+1] = Attr;
}

static void
cgascreenputc(int c)
{
	int i;
	uchar *p;

	if(c == '\n'){
		cgapos /= Width;		/* ordinal of current line */
		cgapos = (cgapos+1)*Width;	/* advance to start of next */
	}
	else if(c == '\t'){
		i = 8 - ((cgapos/2)&7);
		while(i-->0)
			cgascreenputc(' ');
	}
	else if(c == '\b'){
		if(cgapos >= 2)
			cgapos -= 2;
		cgascreenputc(' ');
		cgapos -= 2;
	}
	else{
		CGA[cgapos++] = c;
		CGA[cgapos++] = Attr;
	}
	if(cgapos >= Width*Height){		/* if off-screen, scroll */
		memmove(CGA, &CGA[Width], Width*(Height-1));
		p = &CGA[Width*(Height-1)];
		for(i=0; i<Width/2; i++){
			*p++ = ' ';
			*p++ = Attr;
		}
		cgapos = Width*(Height-1);	/* bottom left of screen */
	}
	movecursor();
}

static void
cgascreenputs(char* s, int n)
{
	if(!islo()){
		/*
		 * Don't deadlock trying to
		 * print in an interrupt.
		 */
		if(!canlock(&cgascreenlock))
			return;
	}
	else
		lock(&cgascreenlock);

	while(n-- > 0)
		cgascreenputc(*s++);

	unlock(&cgascreenlock);
}

char hex[] = "0123456789ABCDEF";

void
cgapost(int code)
{
	uchar *post;

	post = &CGA[Width*Height - Postcodewid*2];  /* bottom right of screen */
	*post++ = hex[(code>>4) & 0x0F];
	*post++ = Attr;
	*post++ = hex[code & 0x0F];
	*post   = Attr;

	outb(Pcode, code);
	/*
	 * adding a delay here caused /dev/sysstat to report as very low, even
	 * 0, m->syscall & m->pfault counts for cpus other than 0 (mar 2018).
	 */
	// delay(100);			/* ensure visibility */
}

void
screeninit(void)
{
	cgapos = cgaregr(0x0E)<<8;
	cgapos |= cgaregr(0x0F);
	cgapos *= 2;

	screenputs = cgascreenputs;
}
