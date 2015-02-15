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
	unsigned long long restorepc;
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
	p->restorepc = u->ip;
	p->sig = sig;
	p->hdlr = hdlr;
	p->u = u;
	nstack++;
	u->ip = (unsigned long long) notecont;
	_NOTED(2);	/* NSAVE: clear note but hold state */
}

static void
notecont(Ureg *u, char *s)
{
	Pcstack *p;
	void(*f)(int, char*, Ureg*);

	p = &pcstack[nstack-1];
	f = p->hdlr;
	u->ip = p->restorepc;
	nstack--;
	(*f)(p->sig, s, u);
	_NOTED(3);	/* NRSTR */
}

#define JMPBUFPC 1
#define JMPBUFSP 0

extern sigset_t	_psigblocked;

typedef struct {
	sigset_t set;
	sigset_t blocked;
	unsigned long long jmpbuf[2];
} sigjmp_buf_amd64;

void
siglongjmp(sigjmp_buf j, int ret)
{
	struct Ureg *u;
	sigjmp_buf_amd64 *jb;

	jb = (sigjmp_buf_amd64*)j;

	if(jb->set)
		_psigblocked = jb->blocked;
	if(nstack == 0 || pcstack[nstack-1].u->sp > jb->jmpbuf[JMPBUFSP])
		longjmp((void*)jb->jmpbuf, ret);
	u = pcstack[nstack-1].u;
	nstack--;
	u->ax = ret;
	if(ret == 0)
		u->ax = 1;
	u->ip = jb->jmpbuf[JMPBUFPC];
	u->sp = jb->jmpbuf[JMPBUFSP] + 8;
	_NOTED(3);	/* NRSTR */
}
