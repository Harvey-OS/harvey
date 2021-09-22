/*
 * Print functions for system call tracing.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "/sys/src/libc/9syscall/sys.h"

static void
fmtrwdata(Fmt* f, char* a, int n, char* suffix)
{
	int i, c;
	char *t;

	if(a == nil){
		fmtprint(f, "0x0%s", suffix);
		return;
	}
	validaddr((ulong)a, n, 0);
	t = smalloc(n+1);
	for(i = 0; i < n; i++) {
		c = a[i];
		t[i] = c > 0x20 && c < 0x7f? c: '.';
	}
	t[n] = 0;
	fmtprint(f, " %#p/\"%s\"%s", a, t, suffix);
	free(t);
}

static void
fmtuserstring(Fmt* f, char* a, char* suffix)
{
	int n;
	char *t;

	if(a == nil){
		fmtprint(f, "0/\"\"%s", suffix);
		return;
	}
	validaddr((ulong)a, 1, 0);
	n = (char*)vmemchr(a, 0, 0x7fffffff) - a + 1;
	if (n < 1)
		return;
	t = smalloc(n+1);
	memmove(t, a, n);
	t[n] = 0;
	fmtprint(f, "%#p/\"%s\"%s", a, t, suffix);
	free(t);
}

void
syscallfmt(int syscallno, ulong pc, va_list list)
{
	int i;
	long l;
	vlong vl;
	Fmt fmt;
	void *v;
	char *a, **argv;

	fmtstrinit(&fmt);
	fmtprint(&fmt, "%uld %s ", up->pid, up->text);

	if(syscallno >= nsyscall)
		fmtprint(&fmt, " %d ", syscallno);
	else
		fmtprint(&fmt, "%s ", sysctab[syscallno]?
			sysctab[syscallno]: "huh?");

	fmtprint(&fmt, "%ulx ", pc);
	if(up->syscalltrace != nil)
		free(up->syscalltrace);

	switch(syscallno){
	case SYSR1:
		fmtprint(&fmt, "%#p", va_arg(list, uintptr));
		break;
	case _ERRSTR:					/* deprecated */
	case CHDIR:
	case EXITS:
	case REMOVE:
		fmtuserstring(&fmt, va_arg(list, char*), "");
		break;
	case BIND:
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtprint(&fmt, "%#ux", va_arg(list, int));
		break;
	case CLOSE:
	case NOTED:
		fmtprint(&fmt, "%d", va_arg(list, int));
		break;
	case DUP:
		i = va_arg(list, int);
		fmtprint(&fmt, "%d %d", i, va_arg(list, int));
		break;
	case ALARM:
		fmtprint(&fmt, "%#lud ", va_arg(list, ulong));
		break;
	case EXEC:
		fmtuserstring(&fmt, va_arg(list, char*), "");
		argv = va_arg(list, char**);
		validalign((uintptr)argv, sizeof(char*));
		for(;;){
			validaddr((ulong)argv, sizeof(char**), 0);
			a = *(char **)argv;
			if(a == nil)
				break;
			fmtprint(&fmt, " ");
			fmtuserstring(&fmt, a, "");
			argv++;
		}
		break;
	case _FSESSION:					/* deprecated */
	case _FSTAT:					/* deprecated */
	case _FWSTAT:					/* obsolete */
		i = va_arg(list, int);
		fmtprint(&fmt, "%d %#p", i, va_arg(list, char*));
		break;
	case FAUTH:
		fmtprint(&fmt, "%d", va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), "");
		break;
	case SEGBRK:
	case RENDEZVOUS:
		fmtprint(&fmt, "%#p ", va_arg(list, void*));
		fmtprint(&fmt, "%#p", va_arg(list, void*));
		break;
	case _MOUNT:					/* deprecated */
		fmtprint(&fmt, "%d ", va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtprint(&fmt, "%#ux ", va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), "");
		break;
	case OPEN:
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtprint(&fmt, "%#ux", va_arg(list, int));
		break;
	case OSEEK:					/* deprecated */
		i = va_arg(list, int);
		l = va_arg(list, long);
		fmtprint(&fmt, "%d %ld %d", i, l, va_arg(list, int));
		break;
	case SLEEP:
		fmtprint(&fmt, "%ld", va_arg(list, long));
		break;
	case _STAT:					/* obsolete */
	case _WSTAT:					/* obsolete */
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtprint(&fmt, "%#p", va_arg(list, char*));
		break;
	case RFORK:
		fmtprint(&fmt, "%#ux", va_arg(list, int));
		break;
	case PIPE:
	case BRK_:
		fmtprint(&fmt, "%#p", va_arg(list, int*));
		break;
	case CREATE:
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		i = va_arg(list, int);
		fmtprint(&fmt, "%#ux %#ux", i, va_arg(list, int));
		break;
	case FD2PATH:
	case FSTAT:
	case FWSTAT:
		i = va_arg(list, int);
		a = va_arg(list, char*);
		fmtprint(&fmt, "%d %#p %lud", i, a, va_arg(list, ulong));
		break;
	case NOTIFY:
	case SEGDETACH:
	case _WAIT:					/* deprecated */
		fmtprint(&fmt, "%#p", va_arg(list, void*));
		break;
	case SEGATTACH:
		fmtprint(&fmt, "%d ", va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		/* fall through */
	case SEGFREE:
	case SEGFLUSH:
		v = va_arg(list, void*);
		fmtprint(&fmt, "%#p %lud", v, va_arg(list, ulong));
		break;
	case UNMOUNT:
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtuserstring(&fmt, va_arg(list, char*), "");
		break;
	case SEMACQUIRE:
	case SEMRELEASE:
		v = va_arg(list, int*);
		fmtprint(&fmt, "%#p %d", v, va_arg(list, int));
		break;
	case TSEMACQUIRE:
		v = va_arg(list, long*);
		fmtprint(&fmt, "%#p %ld", v, va_arg(list, ulong));
		break;
	case SEEK:
		v = va_arg(list, vlong*);
		i = va_arg(list, int);
		vl = va_arg(list, vlong);
		fmtprint(&fmt, "%#p %d %#llux %d", v, i, vl, va_arg(list, int));
		break;
	case FVERSION:
		i = va_arg(list, int);
		fmtprint(&fmt, "%d %d ", i, va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtprint(&fmt, "%lud", va_arg(list, ulong));
		break;
	case WSTAT:
	case STAT:
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		/* fall through */
	case ERRSTR:
	case AWAIT:
		a = va_arg(list, char*);
		fmtprint(&fmt, "%#p %lud", a, va_arg(list, ulong));
		break;
	case MOUNT:
		i = va_arg(list, int);
		fmtprint(&fmt, "%d %d ", i, va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), " ");
		fmtprint(&fmt, "%#ux ", va_arg(list, int));
		fmtuserstring(&fmt, va_arg(list, char*), "");
		break;
	case _READ:					/* deprecated */
	case PREAD:
		i = va_arg(list, int);
		v = va_arg(list, void*);
		fmtprint(&fmt, "%d %#p %ld", i, v, va_arg(list, long));
		if(syscallno == PREAD)
			fmtprint(&fmt, " %lld", va_arg(list, vlong));
		break;
	case _WRITE:					/* deprecated */
	case PWRITE:
		fmtprint(&fmt, "%d ", va_arg(list, int));
		v = va_arg(list, void*);
		l = va_arg(list, long);
		fmtrwdata(&fmt, v, MIN(l, 64), " ");
		fmtprint(&fmt, "%ld", l);
		if(syscallno == PWRITE)
			fmtprint(&fmt, " %lld", va_arg(list, vlong));
		break;
	case NSEC:
		fmtprint(&fmt, "%#p", va_arg(list, vlong*));
		break;
	}

	up->syscalltrace = fmtstrflush(&fmt);
}

void
sysretfmt(int syscallno, va_list list, long ret, uvlong start, uvlong stop)
{
	int i;
	long l;
	void* v;
	Fmt fmt;
	char *a, *errstr;

	fmtstrinit(&fmt);

	if(up->syscalltrace)
		free(up->syscalltrace);

	errstr = "\"\"";
	switch(syscallno){
	case EXEC:
	case SEGBRK:
	case SEGATTACH:
	case RENDEZVOUS:
		if((void *)ret == (void*)-1)
			errstr = up->syserrstr;
		fmtprint(&fmt, " = %#p", (void *)ret);
		break;
	case AWAIT:
		a = va_arg(list, char*);
		l = va_arg(list, ulong);
		if(ret > 0){
			fmtuserstring(&fmt, a, " ");
			fmtprint(&fmt, "%lud = %ld", l, ret);
		}else{
			fmtprint(&fmt, "%#p/\"\" %lud = %ld", a, l, ret);
			errstr = up->syserrstr;
		}
		break;
	case _ERRSTR:
	case ERRSTR:
		a = va_arg(list, char*);
		if(syscallno == _ERRSTR)
			l = 64;
		else
			l = va_arg(list, ulong);
		if(ret > 0){
			fmtuserstring(&fmt, a, " ");
			fmtprint(&fmt, "%lud = %ld", l, ret);
		}else{
			fmtprint(&fmt, "\"\" %lud = %ld", l, ret);
			errstr = up->syserrstr;
		}
		break;
	case FD2PATH:
		i = va_arg(list, int);
		USED(i);
		a = va_arg(list, char*);
		l = va_arg(list, ulong);
		if(ret > 0){
			fmtuserstring(&fmt, a, " ");
			fmtprint(&fmt, "%lud = %ld", l, ret);
		}else{
			fmtprint(&fmt, "\"\" %lud = %ld", l, ret);
			errstr = up->syserrstr;
		}
		break;
	case _READ:
	case PREAD:
		i = va_arg(list, int);		/* fd */
		USED(i);
		v = va_arg(list, void*);
		if(ret > 0)
			fmtrwdata(&fmt, v, MIN(ret, 64), "");
		else{
			fmtprint(&fmt, "/\"\"");
			errstr = up->syserrstr;
		}
		fmtprint(&fmt, " %ld", va_arg(list, long));
		if(syscallno == PREAD)
			fmtprint(&fmt, " %lld", va_arg(list, vlong));
		/* fall through */
	default:
	case ALARM:
	case NSEC:
	case _WRITE:
	case PWRITE:
		if(ret == -1)
			errstr = up->syserrstr;
		fmtprint(&fmt, " = %ld", ret);
		break;
	}
	fmtprint(&fmt, " %s %#llud %#llud\n", errstr, start, stop);
	up->syscalltrace = fmtstrflush(&fmt);
}
