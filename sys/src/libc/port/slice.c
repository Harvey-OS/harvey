/*
 * Copyright (c) 2016, Google Inc., Dan Cross <net!gajendra!cross>
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <u.h>
#include <libc.h>

void
psliceinit(PSlice *slice)
{
	memset(slice, 0, sizeof(*slice));
}

void
psliceclear(PSlice *slice)
{
	slice->len = 0;
	memset(slice->ptrs, 0, sizeof(*slice->ptrs) * slice->capacity);
}

void *
psliceget(PSlice *slice, size_t i)
{
	if (i >= slice->len)
		return nil;
	return slice->ptrs[i];
}

int
psliceput(PSlice *slice, size_t i, void *p)
{
	if (i >= slice->len)
		return 0;
	slice->ptrs[i] = p;
	return 1;
}

int
pslicedel(PSlice *slice, size_t i)
{
	if (i >= slice->len)
		return 0;
	memmove(slice->ptrs + i, slice->ptrs + i + 1,
	        (slice->len - (i + 1)) * sizeof(void *));
	slice->len--;
	return 1;
}

void
psliceappend(PSlice *s, void *p)
{
	void **ps;

	assert(p != nil);
	if (s->len == s->capacity) {
		if (s->capacity == 0)
			s->capacity = 4;
		s->capacity *= 2;
		ps = reallocarray(s->ptrs, s->capacity, sizeof(void *));
		assert(ps != nil);		/* XXX: if size*sizeof(void*) overflows. */
		s->ptrs = ps;
	}
	s->ptrs[s->len] = p;
	s->len++;
}

size_t
pslicelen(PSlice *slice)
{
	return slice->len;
}

void **
pslicefinalize(PSlice *slice)
{
	void **ps;

	ps = reallocarray(slice->ptrs, slice->len, sizeof(void *));
	assert(ps != nil);
	slice->len = 0;
	slice->capacity = 0;
	slice->ptrs = nil;

	return ps;
}

void
pslicedestroy(PSlice *slice)
{
	free(slice->ptrs);
	slice->ptrs = nil;
	slice->capacity = 0;
	slice->len = 0;
}
