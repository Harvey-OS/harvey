/*
 * Copyright (C) 2016 Google Inc.
 * Dan Cross <crossd@gmail.com>
 * See LICENSE for license details.
 */

#include <kmalloc.h>
#include <assert.h>
#include <error.h>
#include <slice.h>

void slice_init(struct slice *slice)
{
	memset(slice, 0, sizeof(*slice));
}

void slice_clear(struct slice *slice)
{
	slice->len = 0;
	memset(slice->ptrs, 0, sizeof(*slice->ptrs) * slice->capacity);
}

void *slice_get(struct slice *slice, size_t i)
{
	if (i >= slice->len)
		return NULL;
	return slice->ptrs[i];
}

bool slice_put(struct slice *slice, size_t i, void *p)
{
	if (i >= slice->len)
		return FALSE;
	slice->ptrs[i] = p;
	return TRUE;
}

bool slice_del(struct slice *slice, size_t i)
{
	if (i >= slice->len)
		return FALSE;
	memmove(slice->ptrs + i, slice->ptrs + i + 1,
	        (slice->len - (i + 1)) * sizeof(void *));
	slice->len--;
	return TRUE;
}

void slice_append(struct slice *s, void *p)
{
	void **ps;

	assert(p != NULL);
	if (s->len == s->capacity) {
		if (s->capacity == 0)
			s->capacity = 4;
		s->capacity *= 2;
		ps = kreallocarray(s->ptrs, s->capacity, sizeof(void *),
				   MEM_WAIT);
		assert(ps != NULL);		/* XXX: if size*sizeof(void*) overflows. */
		s->ptrs = ps;
	}
	s->ptrs[s->len] = p;
	s->len++;
}

size_t slice_len(struct slice *slice)
{
	return slice->len;
}

void **slice_finalize(struct slice *slice)
{
	void **ps;

	ps = kreallocarray(slice->ptrs, slice->len, sizeof(void *), MEM_WAIT);
	assert(ps != NULL);
	slice->len = 0;
	slice->capacity = 0;
	slice->ptrs = NULL;

	return ps;
}

void slice_destroy(struct slice *slice)
{
	kfree(slice->ptrs);
	slice->ptrs = NULL;
	slice->capacity = 0;
	slice->len = 0;
}
