/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

unsigned long
ntohl(unsigned long x)
{
	unsigned long n;
	unsigned char *p;

	n = x;
	p = (unsigned char*)&n;
	return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}

unsigned long
htonl(unsigned long h)
{
	unsigned long n;
	unsigned char *p;

	p = (unsigned char*)&n;
	p[0] = h>>24;
	p[1] = h>>16;
	p[2] = h>>8;
	p[3] = h;
	return n;
}

unsigned short
ntohs(unsigned short x)
{
	unsigned short n;
	unsigned char *p;

	n = x;
	p = (unsigned char*)&n;
	return (p[0]<<8)|p[1];
}

unsigned short
htons(unsigned short h)
{
	unsigned short n;
	unsigned char *p;

	p = (unsigned char*)&n;
	p[0] = h>>8;
	p[1] = h;
	return n;
}
