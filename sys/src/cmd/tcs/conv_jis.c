#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"conv.h"
#include	"kuten.h"
#include	"jis.h"
#include	"hdr.h"

/*
	a state machine for interpreting shift-jis.
	don't blame me -- rob made me do it
*/
void
jisproc(int c, Rune **r, long ninput)
{
	static enum { state0, state1, state2, state3, state4 } state = state0;
	static int set8 = 0;
	static int japan646 = 0;
	static int lastc;
	int n;
	long l;

again:
	switch(state)
	{
	case state0:	/* idle state */
		if(c == ESC){ state = state1; return; }
		if(c < 0) return;
		if(!set8 && (c < 128)){
			if(japan646){
				switch(c)
				{
				case '\\':	emit(0xA5); return;	/* yen */
				case '~':	emit(0xAF); return;	/* spacing macron */
				default:	emit(c); return;
				}
			} else {
				emit(c);
				return;
			}
		}
		lastc = c; state = state4; return;

	case state1:	/* seen an escape */
		if(c == '$'){ state = state2; return; }
		if(c == '('){ state = state3; return; }
		emit(ESC); state = state0; goto again;

	case state2:	/* may be shifting into JIS */
		if((c == '@') || (c == 'B')){
			set8 = 1; state = state0; return;
		}
		emit(ESC); emit('$'); state = state0; goto again;

	case state3:	/* may be shifting out of JIS */
		if((c == 'J') || (c == 'H') || (c == 'B')){
			japan646 = (c == 'J');
			set8 = 0; state = state0; return;
		}
		emit(ESC); emit('('); state = state0; goto again;

	case state4:	/* two part char */
		if(c < 0){
			if(squawk)
				fprint(2, "%s: unexpected EOF in %s\n", argv0, file);
			c = 0x21 | (lastc&0x80);
		}
		if((lastc&0x80) != (c&0x80)){	/* guard against latin1 in jis */
			emit(lastc);
			state = state0;
			goto again;
		}
		if(CANS2J(lastc, c)){	/* ms dos sjis */
			int h = lastc, l = c;
			S2J(h, l);			/* convert to 206 */
			n = h*100 + l - 3232;		/* convert to kuten */
		} else
			n = (lastc&0x7F)*100 + (c&0x7f) - 3232;	/* kuten */
		if((l = tabkuten[n]) == -1){
			nerrors++;
			if(squawk)
				fprint(2, "%s: unknown kuten %d (from 0x%x,0x%x) near byte %ld in %s\n", argv0, n, lastc, c, ninput, file);
			if(!clean)
				emit(BADMAP);
		} else {
			if(l < 0){
				l = -l;
				if(squawk)
					fprint(2, "%s: ambiguous kuten %d (mapped to 0x%x) near byte %ld in %s\n", argv0, n, l, ninput, file);
			}
			emit(l);
		}
		state = state0;
	}
}

void
jis_in(int fd, long *notused, struct convert *out)
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
			jisproc(ibuf[i], &r, nin++);
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
	jisproc(-1, &r, nin);
	if(r > obuf)
		OUT(out, obuf, r-obuf);
}

void
jis_out(Rune *base, int n, long *notused)
{
	char *p;
	int i;
	Rune r;
	static int first = 1;
	static long tab[NRUNE];
	long l;

	USED(notused);
	if(first){
		first = 0;
		for(i = 0; i < NRUNE; i++)
			tab[i] = -1;
		for(i = 0; i < KUTENMAX; i++)
			if((l = tabkuten[i]) != -1){
				if(l < 0)
					tab[-l] = i;
				else
					tab[l] = i;
			}
		for(i = 0; kutendups[i]; i += 2)
			tab[kutendups[i+1]] = kutendups[i];
	}
	nrunes += n;
	p = obuf;
	for(i = 0; i < n; i++){
		r = base[i];
		if(r < 128)
			*p++ = r;
		else {
			if(tab[r] != -1){
				*p++ = 0x80 | (tab[r]/100 + ' ');
				*p++ = 0x80 | (tab[r]%100 + ' ');
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
