#include <alef.h>

void
werrstr(byte *fmt, ...)
{
	byte tmp[ERRLEN];

	doprint(tmp, tmp+ERRLEN, fmt, ...);
	errstr(tmp);
}
