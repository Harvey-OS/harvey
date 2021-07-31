#include "stdinc.h"

#include "9.h"

/*
 * Dummy for now.
 */

int
consPrint(char* fmt, ...)
{
	int len;
	va_list args;
	char buf[ERRMAX];

	/*
	 * To do:
	 * This will be integrated with logging and become 'print'.
	 */
	va_start(args, fmt);
	len = vsnprint(buf, sizeof(buf), fmt, args);
	va_end(args);

	return consWrite(buf, len);
}

int
consVPrint(char* fmt, va_list args)
{
	int len;
	char buf[ERRMAX];

	/*
	 * To do:
	 * This will be integrated with logging and become
	 * something else ('vprint'?).
	 */
	len = vsnprint(buf, sizeof(buf), fmt, args);

	return consWrite(buf, len);
}
