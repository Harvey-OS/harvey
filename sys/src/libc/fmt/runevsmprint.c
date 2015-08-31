/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "fmtdef.h"

static int
runeFmtStrFlush(Fmt *f)
{
	Rune *s;
	int n;

	if(f->start == nil)
		return 0;
	n = (int)(uintptr)f->farg;
	n *= 2;
	s = f->start;
	f->start = realloc(s, sizeof(Rune)*n);
	if(f->start == nil){
		f->farg = nil;
		f->to = nil;
		f->stop = nil;
		free(s);
		return 0;
	}
	f->farg = (void*)(uintptr_t)n;
	f->to = (Rune*)f->start + ((Rune*)f->to - s);
	f->stop = (Rune*)f->start + n - 1;
	return 1;
}

int
runefmtstrinit(Fmt *f)
{
	int n;

	memset(f, 0, sizeof *f);
	f->runes = 1;
	n = 32;
	f->start = malloc(sizeof(Rune)*n);
	if(f->start == nil)
		return -1;
	setmalloctag(f->start, getcallerpc(&f));
	f->to = f->start;
	f->stop = (Rune*)f->start + n - 1;
	f->flush = runeFmtStrFlush;
	f->farg = (void*)(uintptr_t)n;
	f->nfmt = 0;
	return 0;
}

/*
 * print into an allocated string buffer
 */
Rune*
runevsmprint(const char *fmt, va_list args)
{
	Fmt f;
	int n;

	if(runefmtstrinit(&f) < 0)
		return nil;
	//f.args = args;
	va_copy(f.args,args);
	n = dofmt(&f, fmt);
	va_end(f.args);
	if(f.start == nil)		/* realloc failed? */
		return nil;
	if(n < 0){
		free(f.start);
		return nil;
	}
	setmalloctag(f.start, getcallerpc(&fmt));
	*(Rune*)f.to = '\0';
	return f.start;
}
