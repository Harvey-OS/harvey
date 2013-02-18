#include <u.h>
#include <libc.h>

int
runevsnprint(Rune *buf, int len, char *fmt, va_list args)
{
	Fmt f;

	if(len <= 0)
		return -1;
	f.runes = 1;
	f.start = buf;
	f.to = buf;
	f.stop = buf + len - 1;
	f.flush = nil;
	f.farg = nil;
	f.nfmt = 0;
	f.args = args;
	dofmt(&f, fmt);
	*(Rune*)f.to = '\0';
	return (Rune*)f.to - buf;
}
