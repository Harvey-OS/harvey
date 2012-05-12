#include <u.h>
#include <libc.h>
#include "expand.h"

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
};
	
enum {
	Width = 80*2,
	Height = 25,
	Attr = (Black<<4)|Grey,
};

#define cga	((uchar*)0xB8000)
int cgapos;

static uchar
cgaregr(int index)
{
	outb(0x3D4, index);
	return inb(0x3D4+1) & 0xFF;
}

static void
cgaregw(int index, int data)
{
	outb(0x3D4, index);
	outb(0x3D4+1, data);
}

static void
movecursor(void)
{
	cgaregw(0x0E, (cgapos/2>>8) & 0xFF);
	cgaregw(0x0F, cgapos/2 & 0xFF);
	cga[cgapos+1] = Attr;
}

void
cgaputc(int c)
{
	int i;
	uchar *p;

	if(c == '\n'){
		cgapos = cgapos/Width;
		cgapos = (cgapos+1)*Width;
	}
	else if(c == '\t'){
		i = 8 - ((cgapos/2)&7);
		while(i-->0)
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
	if(cgapos >= Width*Height){
		memmove(cga, &cga[Width], Width*(Height-1));
		p = &cga[Width*(Height-1)];
		for(i=0; i<Width/2; i++){
			*p++ = ' ';
			*p++ = Attr;
		}
		cgapos = Width*(Height-1);
	}
	movecursor();
}

void
cgainit(void)
{
	cgapos = cgaregr(0x0E)<<8;
	cgapos |= cgaregr(0x0F);
	cgapos *= 2;
}
