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

#define ULONG_MAX	4294967295UL

uint32_t
strtoul(const char *nptr, char **endptr, int base)
{
	const char *p;
	uint32_t n, nn, m;
	int c, ovfl, neg, v, ndig;

	p = nptr;
	neg = 0;
	n = 0;
	ndig = 0;
	ovfl = 0;

	/*
	 * White space
	 */
	for(;;p++){
		switch(*p){
		case ' ':
		case '\t':
		case '\n':
		case '\f':
		case '\r':
		case '\v':
			continue;
		}
		break;
	}

	/*
	 * Sign
	 */
	if(*p=='-' || *p=='+')
		if(*p++ == '-')
			neg = 1;

	/*
	 * Base
	 */
	if(base==0){
		if(*p != '0')
			base = 10;
		else{
			base = 8;
			if(p[1]=='x' || p[1]=='X')
				base = 16;
		}
	}
 	if(base<2 || 36<base)
		goto Return;
	if(base==16 && *p=='0'){
		if(p[1]=='x' || p[1]=='X')
			if(('0' <= p[2] && p[2] <= '9')
			 ||('a' <= p[2] && p[2] <= 'f')
			 ||('A' <= p[2] && p[2] <= 'F'))
				p += 2;
	}
	/*
	 * Non-empty sequence of digits
	 */
	n = 0;
	m = ULONG_MAX/base;
	for(;; p++,ndig++){
		c = *p;
		v = base;
		if('0'<=c && c<='9')
			v = c - '0';
		else if('a'<=c && c<='z')
			v = c - 'a' + 10;
		else if('A'<=c && c<='Z')
			v = c - 'A' + 10;
		if(v >= base)
			break;
		if(n > m)
			ovfl = 1;
		nn = n*base + v;
		if(nn < n)
			ovfl = 1;
		n = nn;
	}

    Return:
	if(ndig == 0)
		p = nptr;
	if(endptr)
		*endptr = (char*)p;
	if(ovfl)
		return ULONG_MAX;
	if(neg)
		return -n;
	return n;
}
