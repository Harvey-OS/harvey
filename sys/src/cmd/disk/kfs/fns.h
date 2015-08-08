/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "portfns.h"

int32_t beint32(char*);
Chan* chaninit(char*);
void check(Filsys*, int32_t);
int cmd_exec(char*);
void consserve(void);
void confinit(void);
int fsinit(int, int);
void* ialloc(uint32_t);
int nextelem(void);
int32_t number(int, int);
Device scsidev(char*);
int skipbl(int);
void startproc(void (*)(void), char*);
void syncproc(void);
void syncall(void);

int fprint(int, char*, ...);
void wreninit(Device);
int wrencheck(Device);
void wrenream(Device);
int32_t wrensize(Device);
int32_t wrensuper(Device);
int32_t wrenroot(Device);
int wrenread(Device, int32_t, void*);
int wrenwrite(Device, int32_t, void*);

/*
 * macros for compat with bootes
 */
#define localfs 1

#define devgrow(d, s) 0
#define nofree(d, a) 0
#define isro(d) 0

#define superaddr(d) ((*devcall[d.type].super)(d))
#define getraddr(d) ((*devcall[d.type].root)(d))
#define devsize(d) ((*devcall[d.type].size)(d))
#define devwrite(d, a, v) ((*devcall[d.type].write)(d, a, v))
#define devread(d, a, v) ((*devcall[d.type].read)(d, a, v))
