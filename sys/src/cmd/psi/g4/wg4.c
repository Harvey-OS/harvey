#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "../system.h"
#ifdef HIRES
#include "../njerq.h"
#include "runlen.h"

fwr_bm_g4(int fp, Bitmap *bm)
{
	int wid, widm, sw, y;
	Word *l;
	Scan sp, pr;
	T6state t;
	sw = 0;
	wid=bm->r.max.x-bm->r.min.x;	/*dobitmap aranges for %8=0*/
	l = bm->base;
	if(bm->r.max.y > 1)l += (bm->r.min.y-1) * bm->width;
	if(bm->r.min.x > WORDSIZE)l += (bm->r.min.x - WORDSIZE)/WORDSIZE;
	t6winit(&t, fp);
	pr.runN = pr.run;
	pr.run[0] = wid;
	for(y=bm->r.min.y;y<bm->r.max.y;y++, l+=bm->width) {
		switch(sw){
		case 0:
			runlen(l,wid,&sp);
			t6write(&t, &sp, &pr);
			sw=1;
			break;
		case 1:
			runlen(l,wid,&pr);
			t6write(&t, &pr, &sp);
			sw=0;
		}
	}
	t6wclose(&t);
}
#endif
