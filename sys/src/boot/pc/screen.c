#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#define	WIDTH	160
#define	HEIGHT	24
#define SCREEN	((char *)(0xB8000|KZERO))
int pos;

void
screeninit(void)
{
	memset(SCREEN+pos, 0, WIDTH*HEIGHT-pos);
}

void
screenputc(int c)
{
	int i;

	if(c == '\n'){
		pos = pos/WIDTH;
		pos = (pos+1)*WIDTH;
	} else if(c == '\t'){
		i = 8 - ((pos/2)&7);
		while(i-->0)
			screenputc(' ');
	} else if(c == '\b'){
		if(pos >= 2)
			pos -= 2;
		screenputc(' ');
		pos -= 2;
	} else {
		SCREEN[pos++] = c;
		SCREEN[pos++] = 2;
	}
	if(pos >= WIDTH*HEIGHT){
		memmove(SCREEN, &SCREEN[WIDTH], WIDTH*(HEIGHT-1));
		memset(&SCREEN[WIDTH*(HEIGHT-1)], 0, WIDTH);
		pos = WIDTH*(HEIGHT-1);
	}
}

void
screenputs(char *s, int n)
{
	while(n-- > 0)
		screenputc(*s++);
}

int
screenbits(void)
{
	return 4;
}
