/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifdef	PLAN9
#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#else
#include	<stdio.h>
#include	<unistd.h>
#include	"plan9.h"
#endif
#include	"hdr.h"
#include	"conv.h"
#include	"gbk.h"

static void
gbkproc(int c, Rune **r, long input_loc)
{
	static enum { state0, state1 } state = state0;
	static int lastc;
	long ch, cold = c;

	switch(state)
	{
	case state0:	/* idle state */
		if(c < 0)
			return;
		if(c >= 0x80){
			lastc = c;
			state = state1;
			return;
		}
		emit(c);
		return;

	case state1:	/* seen a font spec */
		ch = -1;
		c = lastc<<8 | c;
		if(c >= GBKMIN && c < GBKMAX)
			ch = tabgbk[c - GBKMIN];
		if(ch < 0){
			nerrors++;
			if(squawk)
				EPR "%s: bad gbk glyph %d (from 0x%x,0x%lx) near byte %ld in %s\n", argv0, (c & 0xFF), lastc, cold, input_loc, file);
			if(!clean)
				emit(BADMAP);
			state = state0;
			return;
		}
		emit(ch);
		state = state0;
	}
}

void
gbk_in(int fd, i32 *notused, struct convert *out)
{
	Rune ob[N];
	Rune *r, *re;
	u8 ibuf[N];
	int n, i;
	i32 nin;

	USED(notused);
	r = ob;
	re = ob+N-3;
	nin = 0;
	while((n = read(fd, ibuf, sizeof ibuf)) > 0){
		for(i = 0; i < n; i++){
			gbkproc(ibuf[i], &r, nin++);
			if(r >= re){
				OUT(out, ob, r-ob);
				r = ob;
			}
		}
		if(r > ob){
			OUT(out, ob, r-ob);
			r = ob;
		}
	}
	gbkproc(-1, &r, nin);
	if(r > ob)
		OUT(out, ob, r-ob);
	OUT(out, ob, 0);
}


void
gbk_out(Rune *base, int n, long *notused)
{
	char *p;
	int i;
	Rune r;
	static int first = 1;

	USED(notused);
	if(first){
		first = 0;
		for(i = 0; i < NRUNE; i++)
			tab[i] = -1;
		for(i = GBKMIN; i < GBKMAX; i++)
			tab[tabgbk[i-GBKMIN]] = i;
	}
	nrunes += n;
	p = obuf;
	for(i = 0; i < n; i++){
		r = base[i];
		if(r < 0x80)
			*p++ = r;
		else {
			if(tab[r] != -1){
				r = tab[r];
				*p++ = (r>>8) & 0xFF;
				*p++ = r & 0xFF;
				continue;
			}
			if(squawk)
				EPR "%s: rune 0x%x not in output cs\n", argv0, r);
			nerrors++;
			if(clean)
				continue;
			*p++ = BYTEBADMAP;
		}
	}
	noutput += p-obuf;
	if(p > obuf)
		write(1, obuf, p-obuf);
}
