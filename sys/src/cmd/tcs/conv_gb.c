#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"conv.h"
#include	"gb.h"
#include	"hdr.h"

/*
	a state machine for interpreting gb.
	don't blame me -- rob made me do it
*/
void
gbproc(int c, Rune **r, long ninput)
{
	static enum { state0, state1 } state = state0;
	static int lastc;
	long n, ch, cold = c;

again:
	switch(state)
	{
	case state0:	/* idle state */
		if(c < 0)
			return;
		if(c >= 0xA1){
			lastc = c;
			state = state1;
			return;
		}
		emit(c);
		return;

	case state1:	/* seen a font spec */
		if(c >= 0xA1)
			n = (lastc-0xA0)*100 + (c-0xA0);
		else {
			nerrors++;
			if(squawk)
				fprint(2, "%s: bad gb glyph %d (from 0x%x,0x%x) near byte %ld in %s\n", argv0, n, lastc, cold, ninput, file);
			if(!clean)
				emit(BADMAP);
			state = state0;
			return;
		}
		ch = tabgb[n];
		if(ch < 0){
			nerrors++;
			if(squawk)
				fprint(2, "%s: unknown gb %d (from 0x%x,0x%x) near byte %ld in %s\n", argv0, n, lastc, cold, ninput, file);
			if(!clean)
				emit(BADMAP);
		} else
			emit(ch);
		state = state0;
	}
}

void
gb_in(int fd, long *notused, struct convert *out)
{
	Rune obuf[N];
	Rune *r, *re;
	uchar ibuf[N];
	int n, i;
	long nin;

	USED(notused);
	r = obuf;
	re = obuf+N-3;
	nin = 0;
	while((n = read(fd, ibuf, sizeof ibuf)) > 0){
		for(i = 0; i < n; i++){
			gbproc(ibuf[i], &r, nin++);
			if(r >= re){
				OUT(out, obuf, r-obuf);
				r = obuf;
			}
		}
		if(r > obuf){
			OUT(out, obuf, r-obuf);
			r = obuf;
		}
	}
	gbproc(-1, &r, nin);
	if(r > obuf)
		OUT(out, obuf, r-obuf);
}

void
gb_out(Rune *base, int n, long *notused)
{
	char *p;
	int i;
	Rune r;
	static int first = 1;
	static long tab[NRUNE];

	USED(notused);
	if(first){
		first = 0;
		for(i = 0; i < NRUNE; i++)
			tab[i] = -1;
		for(i = 0; i < GBMAX; i++)
			if(tabgb[i] != -1)
				tab[tabgb[i]] = i;
	}
	nrunes += n;
	p = obuf;
	for(i = 0; i < n; i++){
		r = base[i];
		if(r < 128)
			*p++ = r;
		else {
			if(tab[r] != -1){
				r = tab[r];
				*p++ = 0xA0 + (r/100);
				*p++ = 0xA0 + (r%100);
				continue;
			}
			if(squawk)
				fprint(2, "%s: rune 0x%x not in output cs\n", argv0, r);
			nerrors++;
			if(clean)
				continue;
			*p++ = BADMAP;
		}
	}
	noutput += p-obuf;
	if(p > obuf)
		write(1, obuf, p-obuf);
}
