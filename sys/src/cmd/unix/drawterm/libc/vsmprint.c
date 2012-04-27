#include <u.h>
#include <libc.h>
#include "fmtdef.h"

static int
fmtStrFlush(Fmt *f)
{
	char *s;
	int n;

	if(f->start == nil)
		return 0;
	n = (uintptr)f->farg;
	n *= 2;
	s = (char*)f->start;
	f->start = realloc(s, n);
	if(f->start == nil){
		f->farg = nil;
		f->to = nil;
		f->stop = nil;
		free(s);
		return 0;
	}
	f->farg = (void*)(uintptr)n;
	f->to = (char*)f->start + ((char*)f->to - s);
	f->stop = (char*)f->start + n - 1;
	return 1;
}

int
fmtstrinit(Fmt *f)
{
	int n;

	memset(f, 0, sizeof *f);
	f->runes = 0;
	n = 32;
	f->start = malloc(n);
	if(f->start == nil)
		return -1;
	f->to = f->start;
	f->stop = (char*)f->start + n - 1;
	f->flush = fmtStrFlush;
	f->farg = (void*)(uintptr)n;
	f->nfmt = 0;
	return 0;
}

/*
 * print into an allocated string buffer
 */
char*
vsmprint(char *fmt, va_list args)
{
	Fmt f;
	int n;

	if(fmtstrinit(&f) < 0)
		return nil;
	VA_COPY(f.args,args);
	n = dofmt(&f, fmt);
	VA_END(f.args);
	if(n < 0){
		free(f.start);
		return nil;
	}
	return fmtstrflush(&f);
}
