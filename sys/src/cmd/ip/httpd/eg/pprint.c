#include	"all.h"

void
pprint(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	pagep = doprint(pagep, page+sizeof(page), fmt, arg);
	va_end(arg);
}
