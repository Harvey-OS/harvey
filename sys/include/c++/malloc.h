#ifndef __MALLOC__H
#define __MALLOC__H 1

#include <stddef.h>

extern "C" {
	void *malloc(size_t);
	void free(void *);
	void *realloc(void *, size_t);
	void *calloc(size_t, size_t);
}

#endif
