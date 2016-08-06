/*
 * Copyright (C) 2016 Google Inc.
 * Dan Cross <crossd@gmail.com>
 * See LICENSE for license details.
 */

#pragma once

/*
 * A tracking structure for growing lists of pointers.
 */
struct slice {
	void **ptrs;
	size_t len;
	size_t capacity;
};

void slice_init(struct slice *slice);
void slice_clear(struct slice *slice);
void *slice_get(struct slice *slice, size_t i);
bool slice_put(struct slice *slice, size_t i, void *p);
bool slice_del(struct slice *slice, size_t i);
void slice_append(struct slice *s, void *p);
size_t slice_len(struct slice *slice);
void **slice_finalize(struct slice *slice);
void slice_destroy(struct slice *slice);
