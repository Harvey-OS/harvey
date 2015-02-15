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

#include "/sys/src/libc/9syscall/sys.h"

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
	int n;
	char *t;

	if(a == nil){
		fmtprint(f, "0/\"\"%s", suffix);
		return;
	}
	a = validaddr(a, 1, 0);
	n = ((char*)vmemchr(a, 0, 0x7fffffff) - a) + 1;
	t = smalloc(n);
	memmove(t, a, n);
	t[n] = 0;
	fmtprint(f, "%#p/\"%s\"%s", a, t, suffix);
	free(t);
}

/*
 */
void
syscallfmt(int syscallno, va_list list)
{
	int32_t l;
	uint32_t ul;
	Fmt fmt;
	void *v;
	int64_t vl;
	uintptr p;
	int i[2], len, **ip;
	char *a, **argv;

	fmtstrinit(&fmt);
	fmtprint(&fmt, "%d %s ", up->pid, up->text);

	if(syscallno > nsyscall)
		fmtprint(&fmt, " %d ", syscallno);
	else
		fmtprint(&fmt, "%s ", systab[syscallno].n);

	if(up->syscalltrace != nil)
		free(up->syscalltrace);

	switch(syscallno){
	case SYSR1:
		p = va_arg(list, uintptr);
		fmtprint(&fmt, "%#p", p);
		break;
	case _ERRSTR:					/* deprecated */
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
	case EXECAC:
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%d", i[0]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
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
	case _FSESSION:					/* deprecated */
	case _FSTAT:					/* deprecated */
	case _FWSTAT:					/* obsolete */
		i[0] = va_arg(list, int);
		a = va_arg(list, char*);
		fmtprint(&fmt, "%d %#p", i[0], a);
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
	case _MOUNT:					/* deprecated */
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%d ", i[0]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#ux ", i[0]);
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, "");
		break;
	case OPEN:
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		i[0] = va_arg(list, int);
		fmtprint(&fmt, "%#ux", i[0]);
		break;
	case OSEEK:					/* deprecated */
		i[0] = va_arg(list, int);
		l = va_arg(list, int32_t);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%d %ld %d", i[0], l, i[1]);
		break;
	case SLEEP:
		l = va_arg(list, int32_t);
		fmtprint(&fmt, "%ld", l);
		break;
	case _STAT:					/* obsolete */
	case _WSTAT:					/* obsolete */
		a = va_arg(list, char*);
		fmtuserstring(&fmt, a, " ");
		a = va_arg(list, char*);
		fmtprint(&fmt, "%#p", a);
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
	case _WAIT:					/* deprecated */
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
	case SEMSLEEP:
	case SEMWAKEUP:
		v = va_arg(list, int*);
		fmtprint(&fmt, "%#p", v);
		break;
	case SEMALT:
		ip = va_arg(list, int**);
		i[0] = va_arg(list, int);
		validaddr(ip, sizeof(int*)*i[0], 0);
		fmtprint(&fmt, "%#p %d", ip, i[0]);
		break;
	case SEEK:
		v = va_arg(list, int64_t*);
		i[0] = va_arg(list, int);
		vl = va_arg(list, int64_t);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%#p %d %#llux %d", v, i[0], vl, i[1]);
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
	case _READ:					/* deprecated */
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
	case _WRITE:					/* deprecated */
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
	case ZIOPREAD:
		i[0] = va_arg(list, int);
		v = va_arg(list, void*);
		i[1] = va_arg(list, int);
		ul = va_arg(list, usize);
		vl = va_arg(list, int64_t);
		fmtprint(&fmt, "%d %#p %d %ld %ulld", i[0], v, i[1], ul, vl);
		break;
	case ZIOPWRITE:
		i[0] = va_arg(list, int);
		v = va_arg(list, void*);
		i[1] = va_arg(list, int);
		vl = va_arg(list, int64_t);
		fmtprint(&fmt, "%d %#p %d %ulld", i[0], v, i[1], vl);
		break;
	case ZIOFREE:
		v = va_arg(list, void*);
		i[1] = va_arg(list, int);
		fmtprint(&fmt, "%#p %d", v, i[1]);
	case NIXSYSCALL:
		break;
	}
	up->syscalltrace = fmtstrflush(&fmt);
}

void
sysretfmt(int syscallno, va_list list, Ar0* ar0, uint64_t start,
	  uint64_t stop)
{
	int32_t l;
	void* v;
	Fmt fmt;
	int64_t vl;
	int i, len;
	char *a, *errstr;

	fmtstrinit(&fmt);

	if(up->syscalltrace)
		free(up->syscalltrace);

	errstr = "\"\"";
	switch(syscallno){
	default:
		if(ar0->i == -1)
			errstr = up->errstr;
		fmtprint(&fmt, " = %d", ar0->i);
		break;
	case ALARM:
	case _WRITE:
	case PWRITE:
		if(ar0->l == -1)
			errstr = up->errstr;
		fmtprint(&fmt, " = %ld", ar0->l);
		break;
	case EXEC:
	case EXECAC:
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
	case _ERRSTR:
	case ERRSTR:
		a = va_arg(list, char*);
		if(syscallno == _ERRSTR)
			l = 64;
		else
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
	case _READ:
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
