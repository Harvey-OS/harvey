/*
 * pANS stdio -- vsnprintf
 */
#include "iolib.h"
int vsnprintf(char *buf, int nbuf, const char *fmt, va_list args){
	int n;
	FILE *f=sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, nbuf);
	n=vfprintf(f, fmt, args);
	sclose(f);
	return n;
}
