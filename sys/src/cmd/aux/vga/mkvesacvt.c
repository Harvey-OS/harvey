/*
 * vesa coordinated video timings computation, using only integers.
 * see /public/doc/vesa/cvt1.1.pdf.
 * simplifying assumptions: no interlace, no top nor bottom margins.
 */
#include <u.h>
#include <libc.h>
#include "vesacvt.h"

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))
#define	ROUNDDN(s, sz)	((s) / (sz) * (sz))

#define MIN(a, b)	((a) < (b)? (a): (b))
#define MAX(a, b)	((a) > (b)? (a): (b))

enum {
	KHZ = 1000,
	MHZ = KHZ * KHZ,
	CLOCK_STEP = MHZ / 4,

	MIN_VSYNC_BP	= 550,
	MIN_VBPORCH	= 6,
	MIN_V_PORCH_RND	= 3,

	/* reduced blanking parameters */
	RB_MIN_V_BLANK	= 460,
	RB_V_FPORCH	= 3,
	RB_H_SYNC	= 32,
	RB_H_BLANK	= 160,
};

int
getvsync(ulong wid, ulong ht)
{
	/* work out aspect ratio, thus vsync */
	if (wid * 3 == ht * 4)
		return 4;		/* 4:3 */
	else if (wid * 9 == ht * 16)
		return 5;		/* 16:9 */
	else if (wid * 10 == ht * 16)
		return 6;		/* 16:10 */
	else if (wid * 4 == ht * 5)
		return 7;		/* 5:4 */
	else if (wid * 9 == ht * 15)
		return 7;		/* 15:9 */
	else
		return 10;
}

/*
 * fhperiodest = (1./hz - (double)MIN_VSYNC_BP/MHZ) /
 *	(ht + MIN_V_PORCH_RND) * MHZ; =>
 * fhperiodest = ((double)MHZ/hz - MIN_VSYNC_BP)/(ht + MIN_V_PORCH_RND);
 */

/* hz must be 60, else the results are undefined */
static void
rbcvt(Cvt *cvt)
{
	/*
	 * 2nd arg to MAX was: vbilines = RB_MIN_V_BLANK /
	 *	((MHZ/hz - RB_MIN_V_BLANK) / ht) + 1;
	 */
	cvt->totvlines = cvt->ht + MAX(RB_V_FPORCH + cvt->vsync + MIN_VBPORCH,
		cvt->ht / ((MHZ / (RB_MIN_V_BLANK*cvt->hz) - 1)) + 1);
	cvt->hblank = RB_H_BLANK;
	cvt->totpix = cvt->activehpix + cvt->hblank;
	cvt->pixfreq = ROUNDDN(cvt->hz * cvt->totvlines * cvt->totpix, CLOCK_STEP);
	cvt->vbporch = cvt->totvlines - (cvt->ht + cvt->vsync + cvt->vfporch);
	cvt->hsync = RB_H_SYNC;
}

static void
crtcvt(Cvt *cvt)
{
	ulong vsyncbp, dutycycle;

	/* vsyncbp = MIN_VSYNC_BP/fhperiodest + 1; => */
	vsyncbp = (cvt->ht + MIN_V_PORCH_RND) /
		(MHZ / (MIN_VSYNC_BP*cvt->hz) - 1) + 1;
	if (vsyncbp < cvt->vsync + MIN_VBPORCH)
		vsyncbp = cvt->vsync + MIN_VBPORCH;

	cvt->vbporch = vsyncbp - cvt->vsync;
	cvt->totvlines = cvt->ht + vsyncbp + MIN_V_PORCH_RND;

	/* dutycycle = 30 - (300 * fhperiodest / 1000); => */
	dutycycle = 30 - 3*(MHZ/10/cvt->hz - MIN_VSYNC_BP/10) / (cvt->ht + 3);
	assert(dutycycle > 0 && dutycycle < 100);
	if(dutycycle < 20)
		dutycycle = 20;

	cvt->hblank = ROUNDDN(cvt->activehpix*dutycycle / (100 - dutycycle),
		2*8);
	cvt->totpix = cvt->activehpix + cvt->hblank;
	/*
	 * cvt->pixfreq = ROUNDDN((ulong)(cvt->totpix/fhperiodest * MHZ),
	 * 	CLOCK_STEP); =>
	 */
	cvt->pixfreq = ROUNDDN((ulong)(cvt->totpix *
		(cvt->ht + (vlong)MIN_V_PORCH_RND) * MHZ /
		(MHZ/cvt->hz - MIN_VSYNC_BP)), CLOCK_STEP);
	cvt->hsync = ROUNDDN(8 * cvt->totpix / 100, 8);
}

Cvt *
vesacvt(ulong wid, ulong ht, ulong hz, int reducedblank)
{
	Cvt *cvt;

	cvt = mallocz(sizeof *cvt, 1);
	if (cvt == nil)
		return nil;
	cvt->wid = wid;
	cvt->ht = ht;
	cvt->hz = hz;
	/* hz must be 60 for rb, else the results are undefined */
	cvt->reducedblank = reducedblank && hz == 60;
	cvt->activehpix = ROUNDDN(wid, 8);
	cvt->vsync = getvsync(wid, ht);
	cvt->vfporch = 3;
	if (cvt->reducedblank)
		rbcvt(cvt);
	else
		crtcvt(cvt);
	return cvt;
}
