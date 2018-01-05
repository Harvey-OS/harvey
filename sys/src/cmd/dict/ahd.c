/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dict.h"

/*
 * American Heritage Dictionary (encrypted)
 */

static Rune intab[256] = {
	[0x82] = L'é',
	[0x85] = L'à',
	[0x89] = L'ë',
	[0x8a] = L'è',
	[0xa4] = L'ñ',
	[0xf8] = L'°',
	[0xf9] = L'·',
};

static char	tag[64];

enum{
	Run, Openper, Openat, Closeat
};

void
ahdprintentry(Entry e, int cmd)
{
	static int inited;
	int32_t addr;
	char *p, *t = tag;
	int obreaklen;
	int c, state = Run;

	if(!inited){
		for(c=0; c<256; c++)
			if(intab[c] == 0)
				intab[c] = c;
		inited = 1;
	}
	obreaklen = breaklen;
	breaklen = 80;
	addr = e.doff;
	for(p=e.start; p<e.end; p++){
		c = intab[(*p ^ (addr++>>1))&0xff];
		switch(state){
		case Run:
			if(c == '%'){
				t = tag;
				state = Openper;
				break;
			}
		Putchar:
			if(c == '\n')
				outnl(0);
			else if(c < Runeself)
				outchar(c);
			else
				outrune(c);
			break;

		case Openper:
			if(c == '@')
				state = Openat;
			else{
				outchar('%');
				state = Run;
				goto Putchar;
			}
			break;

		case Openat:
			if(c == '@')
				state = Closeat;
			else if(t < &tag[sizeof tag-1])
				*t++ = c;
			break;

		case Closeat:
			if(c == '%'){
				*t = 0;
				switch(cmd){
				case 'h':
					if(strcmp("EH", tag) == 0)
						goto out;
					break;
				case 'r':
					outprint("%%@%s@%%", tag);
					break;
				}
				state = Run;
			}else{
				if(t < &tag[sizeof tag-1])
					*t++ = '@';
				if(t < &tag[sizeof tag-1])
					*t++ = c;
				state = Openat;
			}
			break;
		}
	}
out:
	outnl(0);
	breaklen = obreaklen;
}

int32_t
ahdnextoff(int32_t fromoff)
{
	static char *patterns[] = { "%@NL@%", "%@2@%", 0 };
	int c, k = 0, state = 0;
	char *pat = patterns[0];
	int32_t defoff = -1;

	if(Bseek(bdict, fromoff, 0) < 0)
		return -1;
	while((c = Bgetc(bdict)) >= 0){
		c ^= (fromoff++>>1)&0xff;
		if(c != pat[state]){
			state = 0;
			continue;
		}
		if(pat[++state])
			continue;
		if(pat = patterns[++k]){	/* assign = */
			state = 0;
			defoff = fromoff-6;
			continue;
		}
		return fromoff-5;
	}
	return defoff;
}

void
ahdprintkey(void)
{
	Bprint(bout, "No pronunciations.\n");
}
