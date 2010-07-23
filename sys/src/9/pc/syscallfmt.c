/*
 * Print functions for system call tracing.
 *
 * TODO: pass ureg->usp and ureg->ax into common code.
 */
#include "u.h"
#include "ureg.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "/sys/src/libc/9syscall/sys.h"

extern char *sysctab[];

extern	int	fmtstrinit(Fmt*);
extern	char*	fmtstrflush(Fmt*);

static void
fmtrwdata(Fmt *f, ulong s, int n, char *suffix)
{
	char *t, *src;
	int i;

	if (!s) {
		fmtprint(f, "0x0%s", suffix);
		return;
	}
	src = (char*)s;
	validaddr(s, n, 0);
	t = smalloc(n+1);
	for(i = 0; i < n; i++)
		if(src[i] > 0x20 && src[i] < 0x7f)
			t[i] = src[i];
		else
			t[i] = '.';

	fmtprint(f, "%ulx/\"%s\"%s", s, t, suffix);
	free(t);
}

static void
fmtuserstring(Fmt *f, ulong s, char *suffix)
{
	char *es, *t, *src;
	int n;

	if (!s){
		fmtprint(f, "0/\"\"%s", suffix);
		return;
	}
	src = (char*)s;
	validaddr(s, 1, 0);
	es = vmemchr(src, 0, 1<<16);
	n = es - src;
	t = smalloc(n + 1);
	memmove(t, src, n);
	t[n] = 0;
	fmtprint(f, "%#ulx/\"%s\"%s", s, t, suffix);
	free(t);
}

void
syscallprint(Ureg *ureg)
{
	ulong *sp;
	int syscallno;
	vlong offset;
	Fmt fmt;
	int len;
	char* a;
	char** argp;

	sp = (ulong*)ureg->usp;
	syscallno = ureg->ax;
	offset = 0;
	fmtstrinit(&fmt);
	fmtprint(&fmt, "%ld %s ", up->pid, up->text);
	/* accomodate process-private system calls */

	if(syscallno > nsyscall)
		fmtprint(&fmt, " %d %#lx ", syscallno, sp[0]);
	else
		fmtprint(&fmt, "%s %#ulx ", sysctab[syscallno], sp[0]);

	if(up->syscalltrace)
		free(up->syscalltrace);

	switch(syscallno) {
	case SYSR1:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case _ERRSTR:
		fmtuserstring(&fmt, sp[1], "");
		break;
	case BIND:
		fmtuserstring(&fmt, sp[1], " ");
		fmtuserstring(&fmt, sp[2], " ");
		fmtprint(&fmt, "%#ulx",  sp[3]);
		break;
	case CHDIR:
		fmtuserstring(&fmt, sp[1], "");
		break;
	case CLOSE:
		fmtprint(&fmt, "%ld", sp[1]);
		break;
	case DUP:
		fmtprint(&fmt, "%#ulx %#ulx", sp[1], sp[2]);
		break;
	case ALARM:
		fmtprint(&fmt, "%#ulx ", sp[1]);
		break;
	case EXEC:
		fmtuserstring(&fmt, sp[1], "");
		evenaddr(sp[2]);
		argp = (char**)sp[2];
		validaddr((ulong)argp, BY2WD, 0);
		while(*argp){
			a = *argp++;
			if(((ulong)argp & (BY2PG-1)) < BY2WD)
				validaddr((ulong)argp, BY2WD, 0);
			if(a == 0)
				break;
			fmtprint(&fmt, " ");
			fmtuserstring(&fmt, (ulong)a, "");
		}
		break;
	case EXITS:
		fmtuserstring(&fmt, sp[1], "");
		break;
	case _FSESSION:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case FAUTH:
		fmtprint(&fmt, "%#ulx", sp[1]);
		fmtuserstring(&fmt, sp[2], "");
		break;
	case _FSTAT:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case SEGBRK:
		fmtprint(&fmt, "%#ulx %#ulx", sp[1], sp[2]);
		break;
	case _MOUNT:
		fmtprint(&fmt, "%ld %ld ", sp[1], sp[2]);
		fmtuserstring(&fmt, sp[3], " ");
		fmtprint(&fmt, "%#ulx ", sp[4]);
		fmtuserstring(&fmt, sp[5], "");
		break;
	case OPEN:
		fmtuserstring(&fmt, sp[1], " ");
		fmtprint(&fmt, "%#ulx", sp[2]);
		break;
	case OSEEK:
		fmtprint(&fmt, "%#ulx %#ulx", sp[1], sp[2]);
		break;
	case SLEEP:
		fmtprint(&fmt, "%ld", sp[1]);
		break;
	case _STAT:
		fmtuserstring(&fmt, sp[1], " ");
		fmtprint(&fmt, "%#ulx %ld", sp[2], sp[3]);
		break;
	case RFORK:
		fmtprint(&fmt, "%#ulx", sp[1] );
		break;
	case PIPE:
		break;
	case CREATE:
		fmtuserstring(&fmt, sp[1], " ");
		fmtprint(&fmt, "%#ulx %#ulx", sp[2], sp[3]);
		break;
	case FD2PATH:
		fmtprint(&fmt, "%ld ", sp[1]);
		break;
	case BRK_:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case REMOVE:
		fmtuserstring(&fmt, sp[1], " ");
		break;
	/* deprecated */
	case _WSTAT:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case _FWSTAT:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case NOTIFY:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case NOTED:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case SEGATTACH:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case SEGDETACH:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case SEGFREE:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case SEGFLUSH:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case RENDEZVOUS:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case UNMOUNT:
		fmtuserstring(&fmt, sp[1], " ");
		break;
	case _WAIT:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case SEMACQUIRE:
		fmtprint(&fmt, "%#ulx %#ulx %ld", sp[1], sp[2], sp[3]);
		break;
	case SEMRELEASE:
		fmtprint(&fmt, "%#ulx %#ulx %ld", sp[1], sp[2], sp[3]);
		break;
	case SEEK:
		fmtprint(&fmt, "%#ulx %#ullx %#ulx", sp[1], *(vlong *)&sp[2], sp[4]);
		break;
	case FVERSION:
		fmtprint(&fmt, "%#ulx %#ulx ", sp[1], sp[2]);
		fmtuserstring(&fmt, sp[5], "");
		break;
	case ERRSTR:
		fmtprint(&fmt, "%#ulx/", sp[1]);
		break;
	case WSTAT:
	case STAT:
		fmtprint(&fmt, "%#ulx ", sp[1]);
		fmtuserstring(&fmt, sp[2], " ");
		fmtprint(&fmt, "%#ulx", sp[3]);
		break;
	case FSTAT:
	case FWSTAT:
		fmtprint(&fmt, "%#ulx %#ulx %#ulx", sp[1], sp[2], sp[3]);
		break;
	case MOUNT:
		fmtprint(&fmt, "%ld %ld ", sp[1], sp[3]);
		fmtuserstring(&fmt, sp[3], " ");
		fmtprint(&fmt, "%#ulx", sp[4]);
		fmtuserstring(&fmt, sp[5], "");
		break;
	case AWAIT:
		break;
	case _READ:
	case PREAD:
		fmtprint(&fmt, "%ld ", sp[1]);
		break;
	case _WRITE:
		offset = -1;
		/* fall through */
	case PWRITE:
		fmtprint(&fmt, "%ld ", sp[1]);
		if (sp[3] < 64)
			len = sp[3];
		else
			len = 64;
		fmtrwdata(&fmt, sp[2], len, " ");
		if(!offset)
			offset = *(vlong *)&sp[4];
		fmtprint(&fmt, "%ld %#llx", sp[3], offset);
		break;
	}
	up->syscalltrace = fmtstrflush(&fmt);
}

void
syscallretprint(Ureg *ureg, int syscallno, uvlong start, uvlong stop)
{
	int errstrlen, len;
	vlong offset;
	char *errstr;
	Fmt fmt;

	fmtstrinit(&fmt);
	errstrlen = 0;
	offset = 0;
	if (ureg->ax != -1)
		errstr = "\"\"";
	else
		errstr = up->errstr;

	if(up->syscalltrace)
		free(up->syscalltrace);

	switch(syscallno) {
	case AWAIT:
		if(ureg->ax > 0){
			fmtuserstring(&fmt, up->s.args[0], " ");
			fmtprint(&fmt, "%ld", up->s.args[1]);
		} else
			fmtprint(&fmt, "%#ulx/\"\" %ld", up->s.args[0],
				up->s.args[1]);
		break;
	case _ERRSTR:
		errstrlen = 64;
	case ERRSTR:
		if(!errstrlen)
			errstrlen = up->s.args[1];
		if(ureg->ax > 0){
			fmtuserstring(&fmt, up->s.args[0], " ");
			fmtprint(&fmt, "%d", errstrlen);
		} else
			fmtprint(&fmt, "\"\" %d", errstrlen);
		break;
	case FD2PATH:
		if(ureg->ax == -1)
			fmtprint(&fmt, "\"\" %ld", up->s.args[2]);
		else {
			fmtuserstring(&fmt, up->s.args[1], " ");
			fmtprint(&fmt, "%d", errstrlen);
		}
		break;
	case _READ:
		offset = -1;
		/* fall through */
	case PREAD:
		if(ureg->ax == -1)
			fmtprint(&fmt, "/\"\" %ld 0x%ullx", up->s.args[2],
				*(vlong *)&up->s.args[3]);
		else {
			if (ureg->ax > 64)
				len = 64;
			else
				len = ureg->ax;
			fmtrwdata(&fmt, up->s.args[1], len, " ");
			if(!offset)
				offset = *(vlong *)&up->s.args[3];
			fmtprint(&fmt, "%ld %#llx", up->s.args[2], offset);
		}
		break;
	}
	if(syscallno == EXEC)
		fmtprint(&fmt, " = %p %s %#ullx %#ullx\n", ureg->ax,
			errstr, start, stop);
	else
		fmtprint(&fmt, " = %ld %s %#ullx %#ullx\n", ureg->ax,
			errstr, start, stop);

	up->syscalltrace = fmtstrflush(&fmt);
}
