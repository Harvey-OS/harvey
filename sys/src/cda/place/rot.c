#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#define WDSZ 8
extern int errfd;

uchar mask[] = {
 0x080,		0x040,		0x020,		0x010,
 0x08,		0x04,		0x02,		0x01
};

void
rot(uchar * bd, int widthd, uchar * bs, int widths, int deltax, int deltay)
{
int xs, ys;
int lvl;
	for(ys = 0; ys < deltay; ++ys) {
		for(xs = 0; xs < deltax; ++xs) {
			lvl = getlvl(bs, xs, ys, widths);
			putlvl(bd, ys, deltax - xs - 1, widthd, lvl);
		}
	}
}

int
getlvl(uchar * b, int x, int y, int width) 
{
	int i;
	i = offset(x, y, width);
	return((b[i] & mask[x%WDSZ]) ? 1 : 0);
}


int
offset(int x, int y, int width)
{
	int wbits;
	wbits = (width*WDSZ);
	return((y*wbits + x)/WDSZ);
}

void
putlvl(uchar * b, int x, int y, int width, int lvl) 
{
	uchar *p;
	p = &b[offset(x, y, width)];
	if(lvl) *p |= mask[x%WDSZ];
	else *p &= ~mask[x%WDSZ];
}
