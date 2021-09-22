#include <u.h>
#include <libc.h>

int
sprint(char *buf, char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
#ifdef notdef
	n = vsnprint(buf, 65536, fmt, args);	/* big number, but sprint is deprecated anyway */
#else	/* compilable on arm64 */
	n = vsnprint(buf, 4000, fmt, args);	/* big number, but sprint is deprecated anyway */
#endif
	va_end(args);
	return n;
}
