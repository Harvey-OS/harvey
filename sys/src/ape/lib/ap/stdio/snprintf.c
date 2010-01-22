/*
 * pANS stdio -- sprintf
 */
#define _C99_SNPRINTF_EXTENSION

#include "iolib.h"

int snprintf(char *buf, size_t nbuf, const char *fmt, ...){
	int n;
	va_list args;
	FILE *f=_IO_sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, nbuf);
	va_start(args, fmt);
	n=vfprintf(f, fmt, args);
	va_end(args);
	_IO_sclose(f);
	return n;
}
