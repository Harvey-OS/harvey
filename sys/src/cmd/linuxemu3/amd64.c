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
	ulong ip;
	ulong sp;
	ulong ax;

	ip = ureg->ip;
	sp = ureg->sp;
	ax = ureg->ax;

	if(!setjmp(jmp))
		notejmp(ureg, jmp, 1);

	ureg->ip = ip;
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
	trace("linuxret(%lux: %s, %lux: %E)", u->ip, p->syscall, (ulong)errno, errno);
	if(errno == -ERESTART){
		p->restart->syscall = p->syscall;
		return;
	}
	u->ax = (ulong)errno;
	u->ip += 2;
	p->restart->syscall = nil;
	p->syscall = nil;
}
int
linuxcall(void)
{
	Uproc *p;
	Ureg *u;
	Linuxcall *c;
	uchar *ip;

	p = current;
	u = p->ureg;

	/* CD 80 = INT 0x80 */
	ip = (uchar*)u->ip;
	if(ip[0] != 0xcd || ip[1] != 0x80){
		trace("linuxcall(): not a syscall ip=%lux sp=%lux", u->ip, u->sp);
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
