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
#include <draw.h>

static char channames[] = "rgbkamx";
char*
chantostr(char *buf, uint32_t cc)
{
	uint32_t c, rc;
	char *p;

	if(chantodepth(cc) == 0)
		return nil;

	/* reverse the channel descriptor so we can easily generate the string in the right order */
	rc = 0;
	for(c=cc; c; c>>=8){
		rc <<= 8;
		rc |= c&0xFF;
	}

	p = buf;
	for(c=rc; c; c>>=8) {
		*p++ = channames[TYPE(c)];
		*p++ = '0'+NBITS(c);
	}
	*p = 0;

	return buf;
}

/* avoid pulling in ctype when using with drawterm etc. */
static int
isspace(char c)
{
	return c==' ' || c== '\t' || c=='\r' || c=='\n';
}

uint32_t
strtochan(char *s)
{
	char *p, *q;
	uint32_t c;
	int t, n, d;

	c = 0;
	d = 0;
	p=s;
	while(*p && isspace(*p))
		p++;

	while(*p && !isspace(*p)){
		if((q = strchr(channames, p[0])) == nil)
			return 0;
		t = q-channames;
		if(p[1] < '0' || p[1] > '9')
			return 0;
		n = p[1]-'0';
		d += n;
		c = (c<<8) | __DC(t, n);
		p += 2;
	}
	if(d==0 || (d>8 && d%8) || (d<8 && 8%d))
		return 0;
	return c;
}

int
chantodepth(uint32_t c)
{
	int n;

	for(n=0; c; c>>=8){
		if(TYPE(c) >= NChan || NBITS(c) > 8 || NBITS(c) <= 0)
			return 0;
		n += NBITS(c);
	}
	if(n==0 || (n>8 && n%8) || (n<8 && 8%n))
		return 0;
	return n;
}
