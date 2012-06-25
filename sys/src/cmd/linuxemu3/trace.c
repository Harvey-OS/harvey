#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"

#undef trace

static char magic[] = "TRACEBUF";

typedef struct Tracebuf Tracebuf;
struct Tracebuf
{
	char		magic[8];
	int		wp;
	char		lines[256][80];
};

static void*
alloctrace(void)
{
	Tracebuf *t;

	t = kmallocz(sizeof(*t), 1);
	memmove(t->magic, magic, sizeof(t->magic));
	return t;
}

static void
checktrace(Tracebuf *t)
{
	if(memcmp(t->magic, magic, sizeof(t->magic)) != 0)
		panic("tracebuffer corrupted");
}

static void
freetrace(Tracebuf *t)
{
	if(t == nil)
		return;
	checktrace(t);
	memset(t, 0, sizeof(*t));
	free(t);
}

static void
vputtrace(Tracebuf *t, char *fmt, va_list a)
{
	char *s;

	checktrace(t);
	s = t->lines[t->wp++ %  nelem(t->lines)];
	vsnprint(s, sizeof(t->lines[0]), fmt, a);
	if(debug > 1)
		fprint(2, "%d\t%s\n", (current != nil) ? current->tid : 0, s);
}

void inittrace(void)
{
	if(debug > 0)
		current->trace = alloctrace();
}

void exittrace(Uproc *proc)
{
	Tracebuf *t;

	if(t = proc->trace){
		proc->trace = nil;
		freetrace(t);
	}
}

void clonetrace(Uproc *new, int copy)
{
	Tracebuf *t;

	if((t = current->trace) == nil){
		new->trace = nil;
		return;
	}

	if(copy){
		Tracebuf *x;

		x = kmalloc(sizeof(*t));
		memmove(x, t, sizeof(*t));
		new->trace = x;

		return;
	}

	new->trace = alloctrace();
}

void tprint(char *fmt, ...)
{
	va_list a;
	Uproc *p;

	p = current;
	if(p && p->trace){
		va_start(a, fmt);
		vputtrace((Tracebuf*)p->trace, fmt, a);
		va_end(a);
	}
}
