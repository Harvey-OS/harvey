#include "lib9.h"

#include <errno.h>

static char errstring[ERRMAX];

enum
{
	Magic = 0xffffff
};

void
werrstr(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vseprint(errstring, errstring+sizeof(errstring), fmt, arg);
	va_end(arg);
	errno = Magic;
}

void
oserrstr(char *buf, uint nerr)
{
	char *s;

	if(errno != EINTR)
		s = strerror(errno);
	else
		s = "interrupted";
	utfecpy(buf, buf+nerr, s);
}

int
errstr(char *buf, uint nerr)
{
	if(errno == Magic)
		utfecpy(buf, buf+nerr, errstring);
	else
		oserrstr(buf, nerr);
	return 1;
}
