#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#define	WIDTH	160
#define	HEIGHT	24
#define SCREEN	((char *)(0xB8000|KZERO))
#define ATTR	0x02

static int inited;
static int pos;

static uchar
cgaregr(uchar index)
{
	outb(0x03D4, index);
	return inb(0x03D4+1);
}

static void
cgaregw(uchar index, uchar data)
{
	outb(0x03D4, index);
	outb(0x03D4+1, data);
}

static void
clearline(int lineno)
{
	char *p;
	int i;

	p = &SCREEN[WIDTH*lineno];
	for(i = 0; i < WIDTH; i += 2){
		*p++ = 0x00;
		*p++ = ATTR;
	}
}

static void
movecursor(void)
{
	cgaregw(0x0E, (pos/2>>8) & 0xFF);
	cgaregw(0x0F, pos/2 & 0xFF);
}

void
cgainit(void)
{
	int i;

	for(i = pos/WIDTH; i < HEIGHT; i++)
		clearline(i);
	movecursor();
	inited = 1;
}

static void
cgaputc(int c)
{
	int i;

	if(c == '\n'){
		if(inited == 0){
			for(i = (pos % WIDTH); i < WIDTH; i += 2)
				cgaputc(' ');
		}
		else{
			pos = pos/WIDTH;
			pos = (pos+1)*WIDTH;
		}
	} else if(c == '\t'){
		i = 4 - ((pos/2)&3);
		while(i-->0)
			cgaputc(' ');
	} else if(c == '\b'){
		if(pos >= 2)
			pos -= 2;
		cgaputc(' ');
		pos -= 2;
	} else {
		SCREEN[pos++] = c;
		SCREEN[pos++] = ATTR;
	}
	if(pos >= WIDTH*HEIGHT){
		memmove(SCREEN, &SCREEN[WIDTH], WIDTH*(HEIGHT-1));
		clearline(HEIGHT-1);
		pos = WIDTH*(HEIGHT-1);
	}
	movecursor();
}

void
cgaputs(IOQ*, char *s, int n)
{
	while(n-- > 0)
		cgaputc(*s++);
}
