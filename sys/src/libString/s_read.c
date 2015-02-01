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
#include "String.h"

enum
{
	Minread=	256,
};

/* Append up to 'len' input bytes to the string 'to'.
 *
 * Returns the number of characters read.
 */ 
extern int
s_read(Biobuf *fp, String *to, int len)
{
	int rv;
	int n;

	if(to->ref > 1)
		sysfatal("can't s_read a shared string");
	for(rv = 0; rv < len; rv += n){
		n = to->end - to->ptr;
		if(n < Minread){
			s_grow(to, Minread);
			n = to->end - to->ptr;
		}
		if(n > len - rv)
			n = len - rv;
		n = Bread(fp, to->ptr, n);
		if(n <= 0)
			break;
		to->ptr += n;
	}
	s_terminate(to);
	return rv;
}
