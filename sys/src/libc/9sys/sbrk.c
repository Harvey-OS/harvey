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

extern	int8_t	end[];
static	int8_t	*bloc = { end };
extern	int	brk_(void*);

enum
{
	Round	= 7
};

int
brk(void *p)
{
	uintptr bl;

	bl = ((uintptr)p + Round) & ~Round;
	if(brk_((void*)bl) < 0)
		return -1;
	bloc = (int8_t*)bl;
	return 0;
}

void*
sbrk(uint32_t n)
{
	uintptr bl;

	bl = ((uintptr)bloc + Round) & ~Round;
	if(brk_((void*)(bl+n)) < 0)
		return (void*)-1;
	bloc = (int8_t*)bl + n;
	return (void*)bl;
}
