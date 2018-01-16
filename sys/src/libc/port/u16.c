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
static char t16e[] = "0123456789ABCDEF";

int
dec16(uint8_t *out, int lim, const char *in, int n)
{
	int c, w = 0, i = 0;
	uint8_t *start = out;
	uint8_t *eout = out + lim;

	while(n-- > 0){
		c = *in++;
		if('0' <= c && c <= '9')
			c = c - '0';
		else if('a' <= c && c <= 'z')
			c = c - 'a' + 10;
		else if('A' <= c && c <= 'Z')
			c = c - 'A' + 10;
		else
			continue;
		w = (w<<4) + c;
		i++;
		if(i == 2){
			if(out + 1 > eout)
				goto exhausted;
			*out++ = w;
			w = 0;
			i = 0;
		}
	}
exhausted:
	return out - start;
}

int
enc16(char *out, int lim, const uint8_t *in, int n)
{
	uint c;
	char *eout = out + lim;
	char *start = out;

	while(n-- > 0){
		c = *in++;
		if(out + 2 >= eout)
			goto exhausted;
		*out++ = t16e[c>>4];
		*out++ = t16e[c&0xf];
	}
exhausted:
	*out = 0;
	return out - start;
}
