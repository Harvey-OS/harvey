#include "lib9.h"
#include "fmtdef.h"

Rune*
runevseprint(Rune *buf, Rune *e, char *fmt, va_list args)
{
	Fmt f;

	if(e <= buf)
		return nil;
	f.runes = 1;
	f.start = buf;
	f.to = buf;
	f.stop = e - 1;
	f.flush = nil;
	f.farg = nil;
	f.nfmt = 0;
	va_copy(f.args, args);
	dofmt(&f, fmt);
	va_end(f.args);
	*(Rune*)f.to = '\0';
	return f.to;
}

