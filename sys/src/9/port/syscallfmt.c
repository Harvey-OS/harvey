/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../../libc/9syscall/sys.h"

/*
 * Print functions for system call tracing.
 */
static void
fmtrwdata(Fmt* f, char* a, int n, char* suffix)
{
	int i;
	char *t;

	if(a == nil){
		fmtprint(f, "0x0%s", suffix);
		return;
	}
	a = validaddr(a, n, 0);
	t = smalloc(n+1);
	for(i = 0; i < n; i++){
		if(a[i] > 0x20 && a[i] < 0x7f)
			t[i] = a[i];
		else
			t[i] = '.';
	}

	fmtprint(f, " %#p/\"%s\"%s", a, t, suffix);
	free(t);
}

static void
fmtuserstring(Fmt* f, char* a, char* suffix)
{
	int len;
	char *t;

	if(a == nil){
		fmtprint(f, "0/\"\"%s", suffix);
		return;
	}
	a = validaddr(a, 1, 0);
	len = ((char*)vmemchr(a, 0, 0x7fffffff) - a);
	t = smalloc(len+1);
	memmove(t, a, len);
	t[len] = 0;
	fmtprint(f, "%#p/\"%s\"%s", a, t, suffix);
	free(t);
}

/*
 */
void
syscallfmt(int syscallno, ...)
{
	Proc *up = externup();
	va_list list;
	int32_t l;
	Fmt fmt;
	void *v;
	int64_t vl;
	int i[2], len;
	char *a, **argv;
	va_start(list, syscallno);
iprint("%d: %d nsyscall %d\n", up->pid, syscallno, nsyscall);
	fmtstrinit(&fmt);
	fmtprint(&fmt, "%d %s ", up->pid, up->text);

	if(syscallno > nsyscall)
		fmtprint(&fmt, " %d ", syscallno);
	else
		fmtprint(&fmt, "%s ", systab[syscallno].n);

	if(up->syscalltrace != nil)
		free(up->syscalltrace);
	up->syscalltrace = nil;

	switch(syscallno){
	case CHDIR:
	case EXITS:
	case REMOVE:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, "");
		break;
	case BIND:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#ux",  i[0]);
		break;
	case CLOSE:
	case NOTED:
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%d", i[0]);
		break;
	case DUP:
		i[0] = va_arg(list, int);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%d %d", i[0], i[1]);
		break;
	case ALARM:
		l = va_arg(list, unsigned long);
		fmtprint(&fmt, "%#lud ", l);
		break;
	case EXEC:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, "");
		argv = va_arg(list, char**);
		evenaddr(PTR2UINT(argv));
		for(;;){
			a = *(char**)validaddr(argv, sizeof(char**), 0);
			if(a == nil)
				break;
			fmtprint(&fmt, " ");
			fmtuserstring(&fmt, a, "");
			argv++;
		}
		break;
	case FAUTH:
		i[0] = va_arg(list, int);
		a = va_arg(list, char*);
		fmtprint(&fmt, "%d", i[0]);
		fmtuserstring(&fmt, a, "");
		break;
	case SEGBRK:
	case RENDEZVOUS:
		v = va_arg(list, void*);
		fmtprint(&fmt, "%#p ", v);
		v = va_arg(list, void*);
		fmtprint(&fmt, "%#p", v);
		break;
	case OPEN:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#ux", i[0]);
		break;
	case RFORK:
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#ux", i[0]);
		break;
	case PIPE:
	case BRK_:
		v = va_arg(list, int*);
		fmtprint(&fmt, "%#p", v);
		break;
	case CREATE:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		i[0] = va_arg(list, int);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%#ux %#ux", i[0], i[1]);
		break;
	case FD2PATH:
	case FSTAT:
	case FWSTAT:
		i[0] = va_arg(list, int);
		a = va_arg(list, char*);
		l = va_arg(list, unsigned long);
		fmtprint(&fmt, "%d %#p %lud", i[0], a, l);
		break;
	case NOTIFY:
	case SEGDETACH:
		v = va_arg(list, void*);
		fmtprint(&fmt, "%#p", v);
		break;
	case SEGATTACH:
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%d ", i[0]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		/*FALLTHROUGH*/
	case SEGFREE:
	case SEGFLUSH:
		v = va_arg(list, void*);
		l = va_arg(list, unsigned long);
		fmtprint(&fmt, "%#p %lud", v, l);
		break;
	case UNMOUNT:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, "");
		break;
	case SEMACQUIRE:
	case SEMRELEASE:
		v = va_arg(list, int*);
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#p %d", v, i[0]);
		break;
	case TSEMACQUIRE:
		v = va_arg(list, int*);
		l = va_arg(list, uint32_t);
		fmtprint(&fmt, "%#p %ld", v, l);
		break;
	case AWAKE:
		vl = va_arg(list, int64_t);
		fmtprint(&fmt, "%lld", vl);
		break;
	case SEEK:
		i[0] = va_arg(list, int);
		vl = va_arg(list, int64_t);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%d %#llux %d", i[0], vl, i[1]);
		break;
	case FVERSION:
		i[0] = va_arg(list, int);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%d %d ", i[0], i[1]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		l = va_arg(list, unsigned long);
		fmtprint(&fmt, "%lud", l);
		break;
	case WSTAT:
	case STAT:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		/*FALLTHROUGH*/
	case ERRSTR:
	case AWAIT:
		a = va_arg(list, char*);
		l = va_arg(list, unsigned long);
		fmtprint(&fmt, "%#p %lud", a, l);
		break;
	case MOUNT:
		i[0] = va_arg(list, int);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%d %d ", i[0], i[1]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#ux ", i[0]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, "");
		break;
	case READ:					/* deprecated */
	case PREAD:
		i[0] = va_arg(list, int);
		v = va_arg(list, void*);
		l = va_arg(list, int32_t);
		fmtprint(&fmt, "%d %#p %ld", i[0], v, l);
		if(syscallno == PREAD){
			vl = va_arg(list, int64_t);
			fmtprint(&fmt, " %lld", vl);
		}
		break;
	case WRITE:					/* deprecated */
	case PWRITE:
		i[0] = va_arg(list, int);
		v = va_arg(list, void*);
		l = va_arg(list, int32_t);
		fmtprint(&fmt, "%d ", i[0]);
		len = MIN(l, 64);
		fmtrwdata(&fmt, v, len, " ");
		fmtprint(&fmt, "%ld", l);
		if(syscallno == PWRITE){
			vl = va_arg(list, int64_t);
			fmtprint(&fmt, " %lld", vl);
		}
		break;
	}
	up->syscalltrace = fmtstrflush(&fmt);
}

void
sysretfmt(int syscallno, Ar0* ar0, uint64_t start,
	  uint64_t stop, ...)
{
	Proc *up = externup();
	va_list list;
	int32_t l;
	void* v;
	Fmt fmt;
	int64_t vl;
	int i, len;
	char *a, *errstr;

	fmtstrinit(&fmt);
	va_start(list, stop);
	if(up->syscalltrace)
		free(up->syscalltrace);
	up->syscalltrace = nil;

	errstr = "\"\"";
	switch(syscallno){
	default:
		if(ar0->i == -1)
			errstr = up->errstr;
		fmtprint(&fmt, " = %d", ar0->i);
		break;
	case AWAKE:
		if(ar0->vl == 0)
			errstr = up->errstr;
		fmtprint(&fmt, " = %lld", ar0->vl);
		break;
	case SEEK:
		if(ar0->vl == -1)
			errstr = up->errstr;
		fmtprint(&fmt, " = %lld", ar0->vl);
		break;
	case NSEC:
		fmtprint(&fmt, " = %lld", ar0->vl);
		break;
	case ALARM:
	case WRITE:
	case PWRITE:
		if(ar0->l == -1)
			errstr = up->errstr;
		fmtprint(&fmt, " = %ld", ar0->l);
		break;
	case EXEC:
	case SEGBRK:
	case SEGATTACH:
	case RENDEZVOUS:
		if(ar0->v == (void*)-1)
			errstr = up->errstr;
		fmtprint(&fmt, " = %#p", ar0->v);
		break;
	case AWAIT:
		a = va_arg(list, char*);
		l = va_arg(list, unsigned long);
		if(ar0->i > 0){
			fmtuserstring(&fmt, a, " ");
			fmtprint(&fmt, "%lud = %d", l, ar0->i);
		}
		else{
			fmtprint(&fmt, "%#p/\"\" %lud = %d", a, l, ar0->i);
			errstr = up->errstr;
		}
		break;
	case ERRSTR:
		a = va_arg(list, char*);
		l = va_arg(list, unsigned long);
		if(ar0->i > 0){
			fmtuserstring(&fmt, a, " ");
			fmtprint(&fmt, "%lud = %d", l, ar0->i);
		}
		else{
			fmtprint(&fmt, "\"\" %lud = %d", l, ar0->i);
			errstr = up->errstr;
		}
		break;
	case FD2PATH:
		i = va_arg(list, int);
		USED(i);
		a = va_arg(list, char*);
		l = va_arg(list, unsigned long);
		if(ar0->i > 0){
			fmtuserstring(&fmt, a, " ");
			fmtprint(&fmt, "%lud = %d", l, ar0->i);
		}
		else{
			fmtprint(&fmt, "\"\" %lud = %d", l, ar0->i);
			errstr = up->errstr;
		}
		break;
	case READ:
	case PREAD:
		i = va_arg(list, int);
		USED(i);
		v = va_arg(list, void*);
		l = va_arg(list, int32_t);
		if(ar0->l > 0){
			len = MIN(ar0->l, 64);
			fmtrwdata(&fmt, v, len, "");
		}
		else{
			fmtprint(&fmt, "/\"\"");
			errstr = up->errstr;
		}
		fmtprint(&fmt, " %ld", l);
		if(syscallno == PREAD){
			vl = va_arg(list, int64_t);
			fmtprint(&fmt, " %lld", vl);
		}
		fmtprint(&fmt, " = %d", ar0->i);
		break;
	}
	fmtprint(&fmt, " %s %#llud %#llud\n", errstr, start, stop);

	up->syscalltrace = fmtstrflush(&fmt);
}
