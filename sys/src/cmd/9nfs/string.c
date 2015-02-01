/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

#define STRHASH	509	/* prime */

static Strnode *	stab[STRHASH];

static long	hashfun(void*);
static Strnode*	nalloc(int);

char *
strfind(char *a)
{
	Strnode **bin, *x, *xp;

	bin = &stab[hashfun(a) % STRHASH];
	for(xp=0, x=*bin; x; xp=x, x=x->next)
		if(x->str[0] == a[0] && strcmp(x->str, a) == 0)
			break;
	if(x == 0)
		return 0;
	if(xp){
		xp->next = x->next;
		x->next = *bin;
		*bin = x;
	}
	return x->str;
}

char *
strstore(char *a)
{
	Strnode **bin, *x, *xp;
	int n;

	bin = &stab[hashfun(a) % STRHASH];
	for(xp=0, x=*bin; x; xp=x, x=x->next)
		if(x->str[0] == a[0] && strcmp(x->str, a) == 0)
			break;
	if(x == 0){
		n = strlen(a)+1;
		x = nalloc(n);
		memmove(x->str, a, n);
		x->next = *bin;
		*bin = x;
	}else if(xp){
		xp->next = x->next;
		x->next = *bin;
		*bin = x;
	}
	return x->str;
}

void
strprint(int fd)
{
	Strnode **bin, *x;

	for(bin = stab; bin < stab+STRHASH; bin++)
		for(x=*bin; x; x=x->next)
			fprint(fd, "%ld %s\n", bin-stab, x->str);
}

static long
hashfun(void *v)
{
	ulong a = 0, b;
	uchar *s = v;

	while(*s){
		a = (a << 4) + *s++;
		if(b = a&0xf0000000){	/* assign = */
			a ^= b >> 24;
			a ^= b;
		}
	}
	return a;
}

#define STRSIZE	1000

static Strnode *
nalloc(int n)	/* get permanent storage for Strnode */
{
	static char *curstp;
	static int nchleft;
	int k;
	char *p;

	if(n < 4)
		n = 4;
	else if(k = n&3)	/* assign = */
		n += 4-k;
	n += sizeof(Strnode)-4;
	if(n > nchleft){
		nchleft = STRSIZE;
		while(nchleft < n)
			nchleft *= 2;
		if((curstp = malloc(nchleft)) == 0)
			panic("malloc(%d) failed in nalloc\n", nchleft);
	}
	p = curstp;
	curstp += n;
	nchleft -= n;
	return (Strnode*)p;
}
