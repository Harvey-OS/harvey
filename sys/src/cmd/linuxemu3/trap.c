#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

#pragma profile off

void
retuser(void)
{
	Uproc *p;
	Ureg *u;

	p = current;
	u = p->ureg;
	p->ureg = nil;
	if(p->innote == 0)
		jumpureg(u);
	p->innote = 0;
	noted(NCONT);
}

static void
handletrap(void *v, char *m)
{
	Uproc *p;
	Usiginfo si;

	p = current;
	p->innote = 1;
	p->ureg = v;

	if(strncmp(m, "interrupt", 9) == 0){
		if(p->notified){
			p->notified = 0;
		} else {
			memset(&si, 0, sizeof(si));
			si.signo = SIGINT;
			sendsignal(p, &si, 0);
		}
		goto handled;
	}

	if(p->traceproc)
		goto traced;

	if(strncmp(m, "sys: trap: general protection violation", 39) == 0)
		if(linuxcall() == 0)
			goto handled;

	if(strncmp(m, "sys: write on closed pipe", 25) == 0)
		goto handled;

	if(strncmp(m, "sys: trap: invalid opcode", 25) == 0){
		memset(&si, 0, sizeof(si));
		si.signo = SIGILL;
		si.code = ILL_ILLOPC;
		si.fault.addr = (void*)p->ureg->ip;
		sendsignal(p, &si, 0);
		goto handled;
	}

	if(strncmp(m, "sys: trap: divide error", 23) == 0){
		memset(&si, 0, sizeof(si));
		si.signo = SIGFPE;
		si.code = FPE_INTDIV;
		si.fault.addr = (void*)p->ureg->ip;
		sendsignal(p, &si, 0);
		goto handled;
	}

	if(strncmp(m, "sys: trap: overflow", 19) == 0){
		memset(&si, 0, sizeof(si));
		si.signo = SIGFPE;
		si.code = FPE_INTOVF;
		si.fault.addr = (void*)p->ureg->ip;
		sendsignal(p, &si, 0);
		goto handled;
	}

	trace("handletrap: %s", m);
	if(debug)
		noted(NDFLT);

	exitproc(p, SIGKILL, 1);

handled:
	if(p->traceproc)
traced:	p->traceproc(p->tracearg);

	handlesignals();
	retuser();
}

#pragma profile on


void inittrap(void)
{
	ulong f;

	/* disable FPU faults */
	f = getfcr();
	f &= ~(FPINEX|FPOVFL|FPUNFL|FPZDIV|FPINVAL);
	setfcr(f);

	notify(handletrap);
}
