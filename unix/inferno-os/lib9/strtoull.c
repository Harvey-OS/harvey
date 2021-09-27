#include "lib9.h"

#define UVLONG_MAX	((uvlong)1<<63)

uvlong
strtoull(const char *nptr, char **endptr, int base)
{
	const char *p;
	uvlong n, nn, m;
	int c, ovfl, v, neg, ndig;

	p = nptr;
	neg = 0;
	n = 0;
	ndig = 0;
	ovfl = 0;

	/*
	 * White space
	 */
	for(;; p++) {
		switch(*p) {
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
	if(*p == '-' || *p == '+')
		if(*p++ == '-')
			neg = 1;

	/*
	 * Base
	 */
	if(base == 0) {
		base = 10;
		if(*p == '0') {
			base = 8;
			if(p[1] == 'x' || p[1] == 'X'){
				p += 2;
				base = 16;
			}
		}
	} else
	if(base == 16 && *p == '0') {
		if(p[1] == 'x' || p[1] == 'X')
			p += 2;
	} else
	if(base < 0 || 36 < base)
		goto Return;

	/*
	 * Non-empty sequence of digits
	 */
	m = UVLONG_MAX/base;
	for(;; p++,ndig++) {
		c = *p;
		v = base;
		if('0' <= c && c <= '9')
			v = c - '0';
		else
		if('a' <= c && c <= 'z')
			v = c - 'a' + 10;
		else
		if('A' <= c && c <= 'Z')
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
		return UVLONG_MAX;
	if(neg)
		return -n;
	return n;
}
