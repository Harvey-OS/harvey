#include <u.h>
#include <libc.h>
#include "fmtdef.h"

static int
runeFmtStrFlush(Fmt *f)
{
	Rune *s;
	int n;

	n = (int)f->farg;
	n += 256;
	f->farg = (void*)n;
	s = f->start;
	f->start = realloc(s, sizeof(Rune)*n);
	if(f->start == nil){
		f->start = s;
		return 0;
	}
	f->to = (Rune*)f->start + ((Rune*)f->to - s);
	f->stop = (Rune*)f->start + n - 1;
	return 1;
}

int
runefmtstrinit(Fmt *f)
{
	int n;

	f->runes = 1;
	n = 32;
	f->start = malloc(sizeof(Rune)*n);
	if(f->start == nil)
		return -1;
	f->to = f->start;
	f->stop = (Rune*)f->start + n - 1;
	f->flush = runeFmtStrFlush;
	f->farg = (void*)n;
	f->nfmt = 0;
	return 0;
}

/*
 * print into an allocated string buffer
 */
Rune*
runevsmprint(char *fmt, va_list args)
{
	Fmt f;
	int n;

	if(runefmtstrinit(&f) < 0)
		return nil;
	f.args = args;
	n = dofmt(&f, fmt);
	if(n < 0)
		return nil;
	*(Rune*)f.to = '\0';
	return f.start;
}
