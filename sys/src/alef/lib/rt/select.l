#include	<alef.h>
#include	"run.h"

extern int 	ALEFxrand;

/*
 * Build a list of IO ready channels, pick a random ready channel and return
 * its index in the list. This drives switch code generated for the select
 */

int
ALEF_selrdy(Chan *f)
{
	if(f->seltst)
		return f->rva != nil || f->free != nil;	/* test for send */
	else
		return f->sva || f->qh;		/* test for receive */
}

uint
ALEF_varselect(void)
{
	Chan *f;
	Task *t;
	int *sema;
	int nrdy, n;
	uint signature;

	t = PROC.tdb->ctask;
	sema = &t->slist->selp;

	for(;;) {
		nrdy = 0;
		*sema = 0;
		for(f = t->slist; f; f = f->sellink)
			if(ALEF_selrdy(f))
				nrdy++;

		if(nrdy == 0) {
			t->Sleep((void**)sema, 1);
			n = 0;
		}
		else {
			ALEFxrand += ALEFxrand;
			if(ALEFxrand < 0)
				ALEFxrand ^= 0x88888EEF;
			n = ALEFxrand%nrdy;
		}
		for(f = t->slist; f; f = f->sellink)
			if(ALEF_selrdy(f) && n-- == 0)
				break 2;
	}
	if(f->qh != nil)
		signature = f->qh->signature;
	else
		signature = f->signature;

	for(f = t->slist; f; f = f->sellink)
		f->selt = nil;
	t->slist = nil;

	return signature;
}

int
ALEF_doselect(void)
{
	Chan *f;
	Task *t;
	int *sema;
	int nrdy, n, idx;

	t = PROC.tdb->ctask;
	sema = &t->slist->selp;

	for(;;) {
		nrdy = 0;
		*sema = 0;
		for(f = t->slist; f; f = f->sellink)
			if(ALEF_selrdy(f))
				nrdy++;

		if(nrdy == 0) {
			t->Sleep((void**)sema, 1);
			n = 0;
		}
		else {
			ALEFxrand += ALEFxrand;
			if(ALEFxrand < 0)
				ALEFxrand ^= 0x88888EEF;
			n = ALEFxrand%nrdy;
		}
		idx = 0;
		for(f = t->slist; f; f = f->sellink) {
			if(ALEF_selrdy(f) && n-- == 0)
				break 2;
			idx++;
		}
	}
	for(f = t->slist; f; f = f->sellink)
		f->selt = nil;
	t->slist = nil;
	return idx;
}
