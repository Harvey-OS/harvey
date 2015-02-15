/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __STRING_H_
#define __STRING_H_
#pragma lib "/$M/lib/ape/libap.a"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *memcpy(void *, const void *, size_t);
extern void* memccpy(void*, void*, int, size_t);
extern void *memmove(void *, const void *, size_t);
extern int8_t *strcpy(int8_t *, const int8_t *);
extern int8_t *strncpy(int8_t *, const int8_t *, size_t);
extern int8_t *strcat(int8_t *, const int8_t *);
extern int8_t *strncat(int8_t *, const int8_t *, size_t);
extern int memcmp(const void *, const void *, size_t);
extern int strcmp(const int8_t *, const int8_t *);
extern int strcoll(const int8_t *, const int8_t *);
extern int8_t* strdup(int8_t*);
extern int strncmp(const int8_t *, const int8_t *, size_t);
extern size_t strxfrm(int8_t *, const int8_t *, size_t);
extern void *memchr(const void *, int, size_t);
extern int8_t *strchr(const int8_t *, int);
extern size_t strcspn(const int8_t *, const int8_t *);
extern int8_t *strpbrk(const int8_t *, const int8_t *);
extern int8_t *strrchr(const int8_t *, int);
extern size_t strspn(const int8_t *, const int8_t *);
extern int8_t *strstr(const int8_t *, const int8_t *);
extern int8_t *strtok(int8_t *, const int8_t *);
extern void *memset(void *, int, size_t);
extern int8_t *strerror(int);
extern size_t strlen(const int8_t *);

#ifdef _REENTRANT_SOURCE
extern int8_t *strerror_r(int, const int8_t *, int);
extern int8_t *strtok_r(int8_t *, const int8_t *, int8_t **);
#endif

#ifdef _BSD_EXTENSION
#include <bsd.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
