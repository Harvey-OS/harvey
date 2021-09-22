#include <sys/types.h>
#include <lib9.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <fmt.h>
#ifndef _SUSV2_SOURCE
#define _SUSV2_SOURCE
#include <inttypes.h>
#undef	_SUSV2_SOURCE
#else
#include <inttypes.h>
#endif

typedef unsigned int u32int;
typedef unsigned long long u64int;

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

extern	uintptr_t getcallerpc(void*);
extern	void*	mallocz(size_t, int);
extern	void	setmalloctag(void*, ulong);

extern int  dec16(uchar *, int, char *, int);
extern int  enc16(char *, int, uchar *, int);
extern int  dec32(uchar *, int, char *, int);
extern int  enc32(char *, int, uchar *, int);
extern int  dec64(uchar *, int, char *, int);
extern int  enc64(char *, int, uchar *, int);

extern	vlong	nsec(void);

extern void sysfatal(char*, ...);
