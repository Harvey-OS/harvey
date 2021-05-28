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
cgacursor(void)
{
	uchar *cga;

	cgaregw(0x0e, (cgapos/2>>8) & 0xff);
	cgaregw(0x0f, cgapos/2 & 0xff);

	cga = CGA;
	cga[cgapos+1] = Attr;
}

static void
cgaputc(int c)
{
	int i;
	uchar *cga, *p;

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

void
cgaconsputs(char* s, int n)
{
	ilock(&cgalock);
	while(n-- > 0)
		cgaputc(*s++);
	iunlock(&cgalock);
}

void
cgapost(int code)
{
	uchar *cga;

	static char hex[] = "0123456789ABCDEF";

	cga = CGA;
	cga[Width*Height-Postcodelen*2] = hex[(code>>4) & 0x0f];
	cga[Width*Height-Postcodelen*2+1] = Attr;
	cga[Width*Height-Postcodelen*2+2] = hex[code & 0x0f];
	cga[Width*Height-Postcodelen*2+3] = Attr;
}

void
cgainit(void)
{
	ilock(&cgalock);
	cgapos = cgaregr(0x0e)<<8;
	cgapos |= cgaregr(0x0f);
	cgapos *= 2;
	cgainitdone = 1;
	iunlock(&cgalock);
}
