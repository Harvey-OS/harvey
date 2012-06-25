#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Linuxcall Linuxcall;

struct Linuxcall
{
	char	*name;
	void	*func;
	int	(*stub)(Ureg *, void *);
};

static int fcall0(Ureg *, void *func){return ((int (*)(void))func)();}
static int fcall1(Ureg *u, void *func){return ((int (*)(int))func)(u->bx);}
static int fcall2(Ureg *u, void *func){return ((int (*)(int, int))func)(u->bx, u->cx);}
static int fcall3(Ureg *u, void *func){return ((int (*)(int, int, int))func)(u->bx, u->cx, u->dx);}
static int fcall4(Ureg *u, void *func){return ((int (*)(int, int, int, int))func)(u->bx, u->cx, u->dx, u->si);}
static int fcall5(Ureg *u, void *func){return ((int (*)(int, int, int, int, int))func)(u->bx, u->cx, u->dx, u->si, u->di);}
static int fcall6(Ureg *u, void *func){return ((int (*)(int, int, int, int, int, int))func)(u->bx, u->cx, u->dx, u->si, u->di, u->bp);}

#include "linuxcalltab.out"

static Linuxcall nocall = {
	.name = "nosys",
	.func = sys_nosys,
	.stub = fcall0,
};

static void
linuxret(int errno)
{
	Uproc *p;
	Ureg *u;

	p = current;
	u = p->ureg;
	trace("linuxret(%lux: %s, %lux: %E)", u->pc, p->syscall, (ulong)errno, errno);
	if(errno == -ERESTART){
		p->restart->syscall = p->syscall;
		return;
	}
	u->ax = (ulong)errno;
	u->pc += 2;
	p->restart->syscall = nil;
	p->syscall = nil;
}


int
linuxcall(void)
{
	Uproc *p;
	Ureg *u;
	Linuxcall *c;
	uchar *pc;

	p = current;
	u = p->ureg;

	/* CD 80 = INT 0x80 */
	pc = (uchar*)u->pc;
	if(pc[0] != 0xcd || pc[1] != 0x80){
		trace("linuxcall(): not a syscall pc=%lux sp=%lux", u->pc, u->sp);
		return -1;
	}
	c = &linuxcalltab[u->ax];
	if(c > &linuxcalltab[nelem(linuxcalltab)-1])
		c = &nocall;
	p->syscall = c->name;
	p->sysret = linuxret;
	if(p->restart->syscall)
		trace("linuxcall(): restarting %s", p->syscall);
	linuxret(c->stub(u, c->func));
	return 0;
}
