#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

enum {
	Crtx		= 0x3D4,

	CGAWIDTH	= 160,
	CGAHEIGHT	= 24,
};

#define CGASCREEN	((uchar*)(0xB8000 | KZERO))

void
crout(int reg, int val)
{
	outb(Crtx, reg);
	outb(Crtx+1, val);
}

void
cgascreenputc(int c)
{
	int i;
	static int pos;

	if(c == '\r')
		return;
	if(c == '\n'){
		pos = pos/CGAWIDTH;
		pos = (pos+1)*CGAWIDTH;
	} else if(c == '\t'){
		i = 8 - ((pos/2)&7);
		while(i-->0)
			cgascreenputc(' ');
	} else if(c == '\b'){
		if(pos >= 2)
			pos -= 2;
		cgascreenputc(' ');
		pos -= 2;
	} else {
		CGASCREEN[pos++] = c;
		CGASCREEN[pos++] = 2;	/* green on black */
	}
	if(pos >= CGAWIDTH*CGAHEIGHT){
		memmove(CGASCREEN, &CGASCREEN[CGAWIDTH], CGAWIDTH*(CGAHEIGHT-1));
		memset(&CGASCREEN[CGAWIDTH*(CGAHEIGHT-1)], 0, CGAWIDTH);
		pos = CGAWIDTH*(CGAHEIGHT-1);
	}
}

static void
puts(char *s, int n)
{
	while(n--)
		cgascreenputc(*s++);
}

int
cgaprint(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = donprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	puts(buf, n);
	return n;
}

void
cgainit(void)
{
	static int done;

	if(done)
		return;
	done = 1;

	crout(0x0A, 0xFF);
	memset(CGASCREEN, 0, CGAWIDTH*CGAHEIGHT);

	consgetc = kbdgetc;
	consputc = cgascreenputc;
	consputs = puts;
}
