#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum {
	Width		= 160,
	Height		= 25,

	Attr		= 7,		/* white on black */
};

#define CGASCREENBASE	((uchar*)KADDR(0xB8000))

static int pos;
static int screeninitdone;

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
	cgaregw(0x0E, (pos/2>>8) & 0xFF);
	cgaregw(0x0F, pos/2 & 0xFF);
	CGASCREENBASE[pos+1] = Attr;
}

static void
cgascreenputc(int c)
{
	int i;

	if(c == '\n'){
		pos = pos/Width;
		pos = (pos+1)*Width;
	}
	else if(c == '\t'){
		i = 8 - ((pos/2)&7);
		while(i-->0)
			cgascreenputc(' ');
	}
	else if(c == '\b'){
		if(pos >= 2)
			pos -= 2;
		cgascreenputc(' ');
		pos -= 2;
	}
	else{
		CGASCREENBASE[pos++] = c;
		CGASCREENBASE[pos++] = Attr;
	}
	if(pos >= Width*Height){
		memmove(CGASCREENBASE, &CGASCREENBASE[Width], Width*(Height-1));
		memset(&CGASCREENBASE[Width*(Height-1)], 0, Width);
		pos = Width*(Height-1);
	}
	movecursor();
}

static void
screeninit(void)
{
	if(screeninitdone == 0){
		pos = cgaregr(0x0E)<<8;
		pos |= cgaregr(0x0F);
		pos *= 2;
		screeninitdone = 1;
	}
}

void
cgascreenputs(char* s, int n)
{
	if(screeninitdone == 0)
		screeninit();
	while(n-- > 0)
		cgascreenputc(*s++);
}
