#include "../plan9/lib.h"
#include "../plan9/sys9.h"
#include <signal.h>
#include <setjmp.h>

/* A stack to hold pcs when signals nest */
#define MAXSIGSTACK 20
typedef struct Pcstack Pcstack;
static struct Pcstack {
	int sig;
	void (*hdlr)(int, char*, Ureg*);
	unsigned long restorepc;
	Ureg *u;
} pcstack[MAXSIGSTACK];
static int nstack = 0;

static void notecont(Ureg*, char*);

void
_notetramp(int sig, void (*hdlr)(int, char*, Ureg*), Ureg *u)
{
	Pcstack *p;

	if(nstack >= MAXSIGSTACK)
		_NOTED(1);	/* nesting too deep; just do system default */
	p = &pcstack[nstack];
	p->restorepc = u->pc;
	p->sig = sig;
	p->hdlr = hdlr;
	p->u = u;
	nstack++;
	u->pc = (unsigned long) notecont;
	_NOTED(2);	/* NSAVE: clear note but hold state */
}

static void
notecont(Ureg *u, char *s)
{
	Pcstack *p;
	void(*f)(int, char*, Ureg*);

	p = &pcstack[nstack-1];
	f = p->hdlr;
	u->pc = p->restorepc;
	nstack--;
	(*f)(p->sig, s, u);
	_NOTED(3);	/* NRSTR */
}

#define JMPBUFPC 1
#define JMPBUFSP 0

extern sigset_t	_psigblocked;

void
siglongjmp(sigjmp_buf j, int ret)
{
	struct Ureg *u;

	if(j[0])
		_psigblocked = j[1];
	if(nstack == 0 || pcstack[nstack-1].u->sp > j[2+JMPBUFSP])
		longjmp(j+2, ret);
	u = pcstack[nstack-1].u;
	nstack--;
	u->r0 = ret;
	if(ret == 0)
		u->r0 = 1;
	u->pc = j[2+JMPBUFPC];
	u->sp = j[2+JMPBUFSP] + 4;
	_NOTED(3);	/* NRSTR */
}
