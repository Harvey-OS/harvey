/*
 * pANS stdio -- vsnprintf
 */
#include "iolib.h"
int vsnprintf(char *buf, int nbuf, const char *fmt, va_list args){
	int n;
	char *v;
	FILE *f=_IO_sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, nbuf);
	n=vfprintf(f, fmt, args);
	_IO_sclose(f);
	return n;
}
