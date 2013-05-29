/*
 * pANS stdio -- vsnprintf
 */
#include "iolib.h"

int vsnprintf(char *buf, size_t nbuf, const char *fmt, va_list args){
	int n;
	FILE *f=_IO_sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, nbuf);
	n=vfprintf(f, fmt, args);
	_IO_sclose(f);
	return n;
}
