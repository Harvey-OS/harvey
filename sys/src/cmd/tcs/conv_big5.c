#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"conv.h"
#include	"big5.h"
#include	"hdr.h"

long big5_a1[] = {
0x20,	0x2c,	0x2ce,	0x2e,	0x2219,	0x2219,	0x3b,	0x3a,
0x3f,	0x21,	0x3a,	0x2026,	0x2025,	0x2c,	0x2ce,	0x2e,
0x2e,	0x3b,	0x3a,	0x3f,	0x21,	0x2503,	0x2015,	0x2502,
0x2500,	0x258e,	0x5f,	0x2307,	-1,	0x28,	0x29,	0x2322,
0x2323,	0x7b,	0x7d,	-1,	-1,	0x28,	0x29,	0x2322,
0x2323,	0x5b,	0x5d,	-1,	-1,	0x226a,	0x226b,	-1,
-1,	0x3c,	0x3e,	0x2227,	0x2228,	0x231c,	0x231f,	0x230d,
0x230e,	0x2554,	0x255d,	0x2557,	0x255a,	0x28,	0x29,	0x7b,
0x7d,	0x5b,	0x5d,	0x312,	0x313,	0x305,	0x2ba,	0x305,
0x2ba,	0x2cb,	0x2ca,	0x22d5,	0x26,	0x2733,	0x203b,	-1,	/* section mark */
0x201d,	0x25cb,	0x25cf,	0x25b3,	0x25b2,	0x25ce,	0x2729,	0x272d,
0x2662,	0x2666,	0x2610,	0x25a0,	0x25bd,	0x25bc,	0x32a3,	0x2105,
0x305,	0x33f,	0x332,	0x333,	0x2504,	0x2508,	0x2504,	0x2508,
-1,	-1,	0x23,	0x26,	0x2a,	0x2b,	0x2d,	0xd7,
0xf7,	0xb1,	0x221a,	0x3c,	0x3e,	0x3d,	0x2266,	0x2267,
0x2260,	0x221e,	0x2252,	0x2261,	0x2b,	0x2d,	0x3c,	0x3e,
0x3d,	0x7e,	0x2229,	0x222a,	0x22a5,	0x2220,	0x221f,	0x22bf,
-1,	-1,	0x222b,	0x222e,	0x2235,	0x2234,	0x2640,	0x2542,
0x2295,	0x2299,	0x2191,	0x2193,	0x2190,	0x2192,	0x2196,	0x2197,
0x2199,	0x2198,	0x2225,	0x2223,	0x2215,
};

/*
	a state machine for interpreting big5 (hk format).
	don't blame me -- rob made me do it
*/
void
big5proc(int c, Rune **r, long ninput)
{
	static enum { state0, state1 } state = state0;
	static int lastc;
	long n, ch, f, cold = c;

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
		if(c == 26)
			c = '\n';
		emit(c);
		return;

	case state1:	/* seen a font spec */
		if(c >= 64 && c <= 126)
			c -= 64;
		else if(c >= 161 && c <= 254)
			c = c-161 + 63;
		else {
			nerrors++;
			if(squawk)
				fprint(2, "%s: bad big5 glyph %d (from 0x%x,0x%x) near byte %ld in %s\n", argv0, n, lastc, cold, ninput, file);
			if(!clean)
				emit(BADMAP);
			state = state0;
			return;
		}
		if(lastc >= 161 && lastc <= 254)
			f = lastc - 162;
		else {
			nerrors++;
			if(squawk)
				fprint(2, "%s: bad big5 font %d (from 0x%x,0x%x) near byte %ld in %s\n", argv0, f, lastc, cold, ninput, file);
			if(!clean)
				emit(BADMAP);
			state = state0;
			return;
		}
		if(f == -1)
			ch = big5_a1[n = c];
		else
			ch = tabbig5[n = f*BIG5FONT + c];
		if(ch < 0){
			nerrors++;
			if(squawk)
				fprint(2, "%s: unknown big5 %d (from 0x%x,0x%x) near byte %ld in %s\n", argv0, n, lastc, cold, ninput, file);
			if(!clean)
				emit(BADMAP);
		} else
			emit(ch);
		state = state0;
	}
}

void
big5_in(int fd, long *notused, struct convert *out)
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
			big5proc(ibuf[i], &r, nin++);
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
	big5proc(-1, &r, nin);
	if(r > obuf)
		OUT(out, obuf, r-obuf);
}

void
big5_out(Rune *base, int n, long *notused)
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
		for(i = 0; i < BIG5MAX; i++)
			if(tabbig5[i] != -1)
				tab[tabbig5[i]] = i;
		for(i = 0; i < BIG5FONT; i++)
			if(big5_a1[i] != -1)
				tab[big5_a1[i]] = BIG5MAX+i;
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
				if(r >= BIG5MAX){
					*p++ = 0xA1;
					*p++ = r-BIG5MAX;
					continue;
				} else {
					*p++ = 0xA2 + (r/BIG5FONT);
					r = r%BIG5FONT;
					if(r <= 62) r += 64;
					else r += 161-63;
					*p++ = r;
					continue;
				}
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
