/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __STDLIB_H
#define __STDLIB_H
#pragma lib "/$M/lib/ape/libap.a"

#include <stddef.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MB_CUR_MAX 4
#define RAND_MAX 32767

typedef struct { int quot, rem; } div_t;
typedef struct { int32_t quot, rem; } ldiv_t;

#ifdef __cplusplus
extern "C" {
#endif

extern double atof(const int8_t *);
extern int atoi(const int8_t *);
extern long int atol(const int8_t *);
extern long long atoll(const int8_t *);
extern double strtod(const int8_t *, int8_t **);
extern long int strtol(const int8_t *, int8_t **, int);
extern unsigned long int strtoul(const int8_t *, int8_t **, int);
extern long long int strtoll(const int8_t *, int8_t **, int);
extern unsigned long long int strtoull(const int8_t *, int8_t **, int);
extern int rand(void);
extern void srand(unsigned int seed);
extern void *calloc(size_t, size_t);
extern void free(void *);
extern void *malloc(size_t);
extern void *realloc(void *, size_t);
extern void abort(void);
extern int atexit(void (*func)(void));
extern void exit(int);
extern int8_t *getenv(const int8_t *);
extern int system(const int8_t *);
extern void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
extern int abs(int);
extern div_t div(int, int);
extern long int labs(long int);
extern ldiv_t ldiv(long int, long int);
extern int mblen(const int8_t *, size_t);
extern int mbtowc(wchar_t *, const int8_t *, size_t);
extern int wctomb(int8_t *, wchar_t);
extern size_t mbstowcs(wchar_t *, const int8_t *, size_t);
extern size_t wcstombs(int8_t *, const wchar_t *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H */
