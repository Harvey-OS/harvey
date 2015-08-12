/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "../plan9/lib.h"
#include "../plan9/sys9.h"
#include <signal.h>
#include <setjmp.h>

/* A stack to hold pcs when signals nest */
#define MAXSIGSTACK 20
typedef struct Pcstack Pcstack;
static struct Pcstack {
	int sig;
	void (*hdlr)(int, char *, Ureg *);
	unsigned long restorepc;
	unsigned long restorenpc;
	Ureg *u;
} pcstack[MAXSIGSTACK];
static int nstack = 0;

static void notecont(Ureg *, char *);

void
_notetramp(int sig, void (*hdlr)(int, char *, Ureg *), Ureg *u)
{
	Pcstack *p;

	if(nstack >= MAXSIGSTACK)
		_NOTED(1); /* nesting too deep; just do system default */
	p = &pcstack[nstack];
	p->restorepc = u->pc;
	p->restorenpc = u->npc;
	p->sig = sig;
	p->hdlr = hdlr;
	p->u = u;
	nstack++;
	u->pc = (unsigned long)notecont;
	u->npc = u->pc + 4;
	_NOTED(2); /* NSAVE: clear note but hold state */
}

static void
notecont(Ureg *u, char *s)
{
	Pcstack *p;
	void (*f)(int, char *, Ureg *);

	p = &pcstack[nstack - 1];
	f = p->hdlr;
	u->pc = p->restorepc;
	u->npc = p->restorenpc;
	nstack--;
	(*f)(p->sig, s, u);
	_NOTED(3); /* NRSTR */
}

int __noterestore(void);

#define JMPBUFPC 1
#define JMPBUFSP 0
#define JMPBUFDPC (-8)

extern sigset_t _psigblocked;

void
siglongjmp(sigjmp_buf j, int ret)
{
	struct Ureg *u;

	if(j[0])
		_psigblocked = j[1];
	if(nstack == 0 || pcstack[nstack - 1].u->sp > j[2 + JMPBUFSP])
		longjmp(j + 2, ret);
	u = pcstack[nstack - 1].u;
	nstack--;
	u->r8 = ret;
	if(ret == 0)
		u->r8 = 1;
	u->r9 = j[JMPBUFPC] - JMPBUFDPC;
	u->pc = (unsigned long)__noterestore;
	u->npc = (unsigned long)__noterestore + 4;
	u->sp = j[JMPBUFSP];
	_NOTED(3); /* NRSTR */
}
