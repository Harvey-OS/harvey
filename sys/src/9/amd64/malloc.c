/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * malloc
 *
 * Wrap malloc functions an Allocator struct to make it easier to test and
 * replace the implementation.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "malloc.h"

static Allocator* _allocator = nil;

void
mallocinitallocator(Allocator* allocator)
{
	_allocator = allocator;
	_allocator->init();
}

void*
malloc(uint32_t size)
{
	// Default implementation of malloc always clears
	return _allocator->mallocz(size, 1);
}

void*
mallocz(uint32_t size, int clr)
{
	return _allocator->mallocz(size, clr);
}

void*
mallocalign(uint32_t nbytes, uint32_t align, int32_t offset, uint32_t span)
{
	return _allocator->mallocalign(nbytes, align, offset, span);
}

void*
realloc(void* ap, uint32_t size)
{
	return _allocator->realloc(ap, size);
}

void*
smalloc(uint32_t size)
{
	Proc* up = externup();
	void* v;

	while((v = _allocator->mallocz(size, 1)) == nil)
		tsleep(&up->sleep, return0, 0, 100);
	return v;
}

void
free(void* ap)
{
	_allocator->free(ap);
}

uint32_t
msize(void* ap)
{
	return _allocator->msize(ap);
}

int32_t
mallocreadsummary(Chan* c, void* a, int32_t n, int32_t offset)
{
	return _allocator->mallocreadsummary(c, a, n, offset);
}

void
mallocsummary(void)
{
	_allocator->mallocsummary();
}

void
setmalloctag(void* v, uint32_t i)
{
}
