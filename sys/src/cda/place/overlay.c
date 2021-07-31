#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

extern Bitmap *grey;;
extern Bitmap screen;
static int mmap[] =
{
	500, 1000, 2500, 5000, 10000
};

void
drawoverlay(void)
{
	int major, minor;
	int hi, lo, x, y;
	Point omaj, omin, p;

	y = muldiv(2000, UNITMAG, scrn.mag);
	for(x = 0; mmap[x] < y; x++);
	major = mmap[x];
	minor = major/5;
	omaj.x = ((scrn.br.origin.x+major-1)/major)*major;
	omaj.y = ((scrn.br.origin.y+major-1)/major)*major;
	omin.x = ((scrn.br.origin.x+minor-1)/minor)*minor;
	omin.y = ((scrn.br.origin.y+minor-1)/minor)*minor;

	lo = scrn.br.origin.y;
	hi = scrn.br.corner.y;
	for(x = omaj.x; x < scrn.br.corner.x; x += major){
		segment(&screen, pbtos(Pt(x, lo)), pbtos(Pt(x, hi)), 0xf, F_XOR);
		for(y = omin.y; y < hi; y += minor){
			if((y-omaj.y)%major == 0) continue;
			p = pbtos(Pt(x, y));
			segment(&screen, Pt(p.x-10, p.y), Pt(p.x+10, p.y), 0xf, F_XOR);
		}
	}
	lo = scrn.br.origin.x;
	hi = scrn.br.corner.x;
	for(y = omaj.y; y < scrn.br.corner.y; y += major){
		segment(&screen, pbtos(Pt(lo, y)), pbtos(Pt(hi, y)), 0xf, F_XOR);
		for(x = omin.x; x < hi; x += minor){
			if((x-omaj.x)%major == 0) continue;
			p = pbtos(Pt(x, y));
			segment(&screen, Pt(p.x, p.y-10), Pt(p.x, p.y+10), 0xf, F_XOR);
		}
	}
}
