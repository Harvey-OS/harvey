/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <lib9.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <fmt.h>

typedef unsigned int u32int;
typedef unsigned long long u64int;

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

extern	ulong	getcallerpc(void*);
extern	void*	mallocz(ulong, int);
extern	void	setmalloctag(void*, ulong);

extern int  dec16(uchar *, int, char *, int);
extern int  enc16(char *, int, uchar *, int);
extern int  dec32(uchar *, int, char *, int);
extern int  enc32(char *, int, uchar *, int);
extern int  dec64(uchar *, int, char *, int);
extern int  enc64(char *, int, uchar *, int);

extern	vlong	nsec(void);

extern void sysfatal(char*, ...);
