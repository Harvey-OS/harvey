/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <errno.h>
#include "sys9.h"

int8_t	end[];
static	int8_t	*bloc = { end };
extern	int	_BRK_(void*);

int8_t *
brk(int8_t *p)
{
	unsigned long n;

	n = (unsigned long)p;
	n += 3;
	n &= ~3;
	if(_BRK_((void*)n) < 0){
		errno = ENOMEM;
		return (int8_t *)-1;
	}
	bloc = (int8_t *)n;
	return 0;
}

void *
sbrk(unsigned long n)
{
	n += 3;
	n &= ~3;
	if(_BRK_((void *)(bloc+n)) < 0){
		errno = ENOMEM;
		return (void *)-1;
	}
	bloc += n;
	return (void *)(bloc-n);
}
