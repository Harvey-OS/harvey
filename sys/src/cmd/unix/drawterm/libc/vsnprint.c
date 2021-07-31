#include <u.h>
#include <libc.h>

int
vsnprint(char *buf, int len, char *fmt, va_list args)
{
	Fmt f;

	if(len <= 0)
		return -1;
	f.runes = 0;
	f.start = buf;
	f.to = buf;
	f.stop = buf + len - 1;
	f.flush = nil;
	f.farg = nil;
	f.nfmt = 0;
	f.args = args;
	dofmt(&f, fmt);
	*(char*)f.to = '\0';
	return (char*)f.to - buf;
}
