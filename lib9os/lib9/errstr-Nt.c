#include "lib9.h"
#include <Windows.h>

static char	errstring[ERRMAX];

enum
{
	Magic = 0xffffff
};

static void
winerror(int e, char *buf, uint nerr)
{
	int r;
	char buf2[ERRMAX], *p, *q;
	
	r = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf2, sizeof(buf2), 0);

	if(r == 0)
		snprint(buf2, ERRMAX, "windows error %d", e);

	q = buf2;
	for(p = buf2; *p; p++) {
		if(*p == '\r')
			continue;
		if(*p == '\n')
			*q++ = ' ';
		else
			*q++ = *p;
	}
	*q = '\0';
	utfecpy(buf, buf+nerr, buf2);
}

void
werrstr(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vseprint(errstring, errstring+sizeof(errstring), fmt, arg);
	va_end(arg);
	SetLastError(Magic);
}

int
errstr(char *buf, uint nerr)
{
	DWORD le;

	le = GetLastError();
	if(le == Magic)
		utfecpy(buf, buf+nerr, errstring);
	else
		winerror(le, buf, nerr);
	return 1;
}

void
oserrstr(char *buf, uint nerr)
{
	winerror(GetLastError(), buf, nerr);
}
