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

Tracebuf *trace = nil;

static void*
alloctrace(void)
{
	Tracebuf *t;

	t = mallocz(sizeof(*t), 1);
	memmove(t->magic, magic, sizeof(t->magic));
	return t;
}

static void
checktrace(Tracebuf *t)
{
	if(memcmp(t->magic, magic, sizeof(t->magic)) != 0)
		exits("tracebuffer corrupted");
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
		fprint(2, "%s\n", s);
}

void inittrace(void)
{
	if(debug > 0)
	  trace = alloctrace();
}

/* what to do? */
void exittrace(void)
{
}

/* er, what? */
void clonetrace(void, void)
{

}

void tprint(char *fmt, ...)
{
	va_list a;

	if(trace){
		va_start(a, fmt);
		vputtrace(trace, fmt, a);
		va_end(a);
	}
}
