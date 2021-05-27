/*
 * Support for a SPI LCD panel from Adafruit
 * based on HX8357D controller chip
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

enum {
	TFTWidth = 480,
	TFTHeight = 320,
};

static void pitftblank(int);
static void pitftdraw(Rectangle);
static long pitftread(Chan*, void*, long, vlong);
static long pitftwrite(Chan*, void*, long, vlong);
static void spicmd(uchar);
static void spidata(uchar *, int);
static void setwindow(int, int, int, int);
static void xpitftdraw(void *);

extern Memimage xgscreen;
extern Lcd *lcd;

static Lcd pitft = {
	pitftdraw,
	pitftblank,
};

static Queue *updateq = nil;

void
pitftlink(void)
{
	addarchfile("pitft", 0666, pitftread, pitftwrite);
}

static void
pitftsetup(void)
{
	uchar spibuf[32];

	gpiosel(25, Output);
	spirw(0, spibuf, 1);
	spicmd(0x01);
	delay(10);
	spicmd(0x11);
	delay(10);
	spicmd(0x29);
	spicmd(0x13);
	spicmd(0x36);
	spibuf[0] = 0xe8;
	spidata(spibuf, 1);
	spicmd(0x3a);
	spibuf[0] = 0x05;
	spidata(spibuf, 1);
}

static long
pitftread(Chan *, void *, long, vlong)
{
	return 0;
}

static long
pitftwrite(Chan *, void *a, long n, vlong)
{
	if(strncmp(a, "init", 4) == 0 && updateq == nil) {
		/*
		 * The HX8357 datasheet shows minimum
		 * clock cycle time of 66nS but the clock high
		 * and low times as 15nS and it seems to
		 * work at around 32MHz.
		 */
		spiclock(32);
		pitftsetup();
		updateq = qopen(16384, 1, nil, nil);
		kproc("pitft", xpitftdraw, nil);
		lcd = &pitft;
	}
	return n;
}

static void
pitftblank(int blank)
{
	USED(blank);
}

static void
pitftdraw(Rectangle r)
{
	if(updateq == nil)
		return;
	if(r.min.x > TFTWidth || r.min.y > TFTHeight)
		return;
	/*
	 * using qproduce to make sure we don't block
	 * but if we've got a lot on the queue, it means we're
	 * redrawing the same areas over and over; clear it
	 * out and just draw the whole screen once
	 */
	if(qproduce(updateq, &r, sizeof(Rectangle)) == -1) {
		r = Rect(0, 0, TFTWidth, TFTHeight);
		qflush(updateq);
		qproduce(updateq, &r, sizeof(Rectangle));
	}
}

int
overlap(Rectangle r1, Rectangle r2)
{
	if(r1.max.x < r2.min.x)
		return 0;
	if(r1.min.x > r2.max.x)
		return 0;
	if(r1.max.y < r2.min.y)
		return 0;
	if(r1.min.y > r2.max.y)
		return 0;
	return 1;
}

int
min(int x, int y)
{
	if(x < y)
		return x;
	return y;
}

int
max(int x, int y)
{
	if(x < y)
		return y;
	return x;
}

/*
 * Because everyone wants to be holding locks when
 * they update the screen but we need to sleep in the
 * SPI code, we're decoupling this into a separate kproc().
 */
static void
xpitftdraw(void *)
{
	Rectangle rec, bb;
	Point pt;
	uchar *p;
	int i, r, c, gotrec;
	uchar spibuf[32];

	gotrec = 0;
	qread(updateq, &rec, sizeof(Rectangle));
	bb = Rect(0, 0, TFTWidth, TFTHeight);
	while(1) {
		setwindow(bb.min.x, bb.min.y,
			bb.max.x-1, bb.max.y-1);
		spicmd(0x2c);
		for(r = bb.min.y; r < bb.max.y; ++r) {
			for(c = bb.min.x; c < bb.max.x; c += 8) {
				for(i = 0; i < 8; ++i) {
					pt.y = r;
					pt.x = c + i;
					p = byteaddr(&xgscreen, pt);
					switch(xgscreen.depth) {
					case 16:		// RGB16
						spibuf[i*2+1] = p[0];
						spibuf[i*2] = p[1];
						break;
					case 24:		// BGR24
						spibuf[i*2] = (p[2] & 0xf8) | 
							(p[1] >> 5);
						spibuf[i*2+1] = (p[0] >> 3) |
							(p[1] << 3);
						break;
					case 32:		// ARGB32
						spibuf[i*2] = (p[0] & 0xf8) | 
							(p[1] >> 5);
						spibuf[i*2+1] = (p[1] >> 3) |
							(p[1] << 3);
						break;
					}
				}
				spidata(spibuf, 16);
			}
		}
		bb.max.y = -1;
		while(1) {
			if(!gotrec) {
				qread(updateq, &rec, sizeof(Rectangle));
				gotrec = 1;
			}
			if(bb.max.y != -1) {
				if(!overlap(bb, rec))
					break;
				rec.min.x = min(rec.min.x, bb.min.x);
				rec.min.y = min(rec.min.y, bb.min.y);
				rec.max.x = max(rec.max.x, bb.max.x);
				rec.max.y = max(rec.max.y, bb.max.y);
			}
			gotrec = 0;
			// Expand rows to 8 pixel alignment
			bb.min.x = rec.min.x & ~7;
			if(bb.min.x < 0)
				bb.min.x = 0;
			bb.max.x = (rec.max.x + 7) & ~7;
			if(bb.max.x > TFTWidth)
				bb.max.x = TFTWidth;
			bb.min.y = rec.min.y;
			if(bb.min.y < 0)
				bb.min.y = 0;
			bb.max.y = rec.max.y;
			if(bb.max.y > TFTHeight)
				bb.max.y = TFTHeight;
			if(qcanread(updateq)) {
				qread(updateq, &rec, sizeof(Rectangle));
				gotrec = 1;
			}
			else
				break;
		}
	}
}

static void
spicmd(uchar c)
{
	char buf;

	gpioout(25, 0);
	buf = c;
	spirw(0, &buf, 1);
}

static void
spidata(uchar *p, int n)
{
	char buf[128];

	if(n > 128)
		n = 128;
	gpioout(25, 1);
	memmove(buf, p, n);
	spirw(0, buf, n);
	gpioout(25, 0);
}

static void
setwindow(int minc, int minr, int maxc, int maxr)
{
	uchar spibuf[4];

	spicmd(0x2a);
	spibuf[0] = minc >> 8;
	spibuf[1] = minc & 0xff;
	spibuf[2] = maxc >> 8;
	spibuf[3] = maxc & 0xff;
	spidata(spibuf, 4);
	spicmd(0x2b);
	spibuf[0] = minr >> 8;
	spibuf[1] = minr & 0xff;
	spibuf[2] = maxr >> 8;
	spibuf[3] = maxr & 0xff;
	spidata(spibuf, 4);
}
