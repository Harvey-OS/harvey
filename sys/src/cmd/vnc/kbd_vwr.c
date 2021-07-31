#include "vnc.h"
#include <keyboard.h>

enum {
	Xshift = 0xFFE1,
	Xctl = 0xFFE3,
	Xmeta = 0xFFE7,
	Xalt = 0xFFE9
};

static struct {
	Rune kbdc;
	ulong keysym;
} ktab[] = {
	{'\b',		0xff08},
	{'\t',		0xff09},
	{'\n',		0xff0d},
	/* {0x0b, 0xff0b}, */
	{'\r',		0xff0d},
	{0x1b,	0xff1b},	/* escape */
	{Kins,	0xff63},
	{0x7F,	0xffff},
	{Khome,	0xff50},
	{Kend,	0xff57},
	{Kpgup,	0xff55},
	{Kpgdown,	0xff56},
	{Kleft,	0xff51},
	{Kup,	0xff52},
	{Kright,	0xff53},
	{Kdown,	0xff54},
	{KF|1,	0xffbe},
	{KF|2,	0xffbf},
	{KF|3,	0xffc0},
	{KF|4,	0xffc1},
	{KF|5,	0xffc2},
	{KF|6,	0xffc3},
	{KF|7,	0xffc4},
	{KF|8,	0xffc5},
	{KF|9,	0xffc6},
	{KF|10,	0xffc7},
	{KF|11,	0xffc8},
	{KF|12,	0xffc9},
};

ulong
runetoksym(Rune r)
{
	int i;

	for(i=0; i<nelem(ktab); i++)
		if(ktab[i].kbdc == r)
			return ktab[i].keysym;
	return r;
}

static void
keyevent(Vnc *v, ulong ksym, int down)
{
	vnclock(v);
	vncwrchar(v, MKey);
	vncwrchar(v, down);
	vncwrshort(v, 0);
	vncwrlong(v, ksym);
	vncflush(v);
	vncunlock(v);
}

void
readkbd(Vnc *v)
{
	char buf[256], k[10];
	int ctlfd, fd, kr, kn, ks, w, shift, ctl, alt;
	Rune r;

	snprint(buf, sizeof buf, "%s/cons", display->devdir);
	if((fd = open(buf, OREAD)) < 0)
		sysfatal("open %s: %r", buf);

	snprint(buf, sizeof buf, "%s/consctl", display->devdir);
	if((ctlfd = open(buf, OWRITE)) < 0)
		sysfatal("open %s: %r", buf);
	write(ctlfd, "rawon", 5);

	kn = 0;
	shift = alt = ctl = 0;
	for(;;) {
		while(!fullrune(k, kn)){
			kr = read(fd, k+kn, sizeof k - kn);
			if(kr <= 0)
				sysfatal("bad read from kbd\n");
			kn += kr;
		}
		w = chartorune(&r, k);
		kn -= w;
		memmove(k, &k[w], kn);
		ks = runetoksym(r);

		switch(r) {
		case Kalt:
			alt = !alt;
			keyevent(v, Xalt, alt);
			break;
		case Kctl:
			ctl = !ctl;
			keyevent(v, Xctl, ctl);
			break;
		case Kshift:
			shift = !shift;
			keyevent(v, Xshift, shift);
			break;
		default:
			if(r == ks && r < 0x1A) {	/* control key */
				keyevent(v, Xctl, 1);
				keyevent(v, r+0x60, 1);	/* 0x60: make capital letter */
				keyevent(v, Xctl, 0);
			} else
				keyevent(v, ks, 1);

			if(alt) {
				keyevent(v, Xalt, 0);
				alt = 0;
			}
			if(ctl) {
				keyevent(v, Xctl, 0);
				ctl = 0;
			}
			if(shift) {
				keyevent(v, Xshift, 0);
				shift = 0;
			}
			break;
		}
	}
}

