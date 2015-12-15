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

//extern	char	end[];
static	char	*bloc;
extern	int	brk_(void*);

enum
{
	Round	= 7
};

int
brk(void *p)
{
	uintptr bl;
	
	if(!bloc)
		bloc = segbrk(nil, nil);

	bl = ((uintptr)p + Round) & ~Round;
	//p = segbrk(p, (void*)bl);
	p = segbrk(bloc, (void*)bl);
	if((uintptr)p == -1)
		return -1;
	bloc = (char*)bl;
	return 0;
}

void*
sbrk(uint32_t n)
{
	uintptr bl;
	void *p;

	if(!bloc)
		bloc = segbrk(nil, nil);

	bl = ((uintptr)bloc + Round) & ~Round;
	//p = segbrk(bloc, (void*)(bl+n));
	p = segbrk(bloc, (void*)(bl+n));
	if((uintptr)p == -1)
		return p;
	bloc = (char*)bl + n;
	return (void*)bl;
}
