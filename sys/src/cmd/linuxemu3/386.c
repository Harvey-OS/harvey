#include <u.h>
#include <libc.h>
#include <ureg.h>
#include <tos.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

void
clinote(struct Ureg *ureg)
{
	jmp_buf jmp;
	ulong pc;
	ulong sp;
	ulong ax;

	pc = ureg->pc;
	sp = ureg->sp;
	ax = ureg->ax;

	if(!setjmp(jmp))
		notejmp(ureg, jmp, 1);

	ureg->pc = pc;
	ureg->sp = sp;
	ureg->ax = ax;
}

void
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

