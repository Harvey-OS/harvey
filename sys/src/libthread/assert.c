#include <u.h>
#include <libc.h>
#include "assert.h"
#include "threadimpl.h"


int _threaddebuglevel = 0;

void
_threaddebug(ulong l, char *fmt, ...)
{
	va_list arg;
	Proc *p;
	int n;
	char buf[256];

	p = *procp;
	if ((l & _threaddebuglevel) == 0) return;
	if(p->curthread)
		n = sprint(buf, "%d.%d ", p->pid, p->curthread->id);
	else
		n = sprint(buf, "%d.nothread ", p->pid);

	va_start(arg, fmt);
	n = doprint(buf+n, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	write(2, buf, n);
	write(2, "\n", 1);
}

void
_threadassert(char *s)
{
	char buf[256];
	Proc *p;

	if (procp && (p = *procp) && p->curthread){
		sprint(buf, "%d.%d ", p->pid, p->curthread->id);
		write(2, buf, strlen(buf));
	}
	snprint(buf, sizeof(buf), "libthread: %s: assertion failed\n", s);
	write(2, buf, strlen(buf));
	notify(nil);
	abort();
}
