/*
 * pANS stdio -- sprintf
 */
#include "iolib.h"
int sprintf(char *buf, const char *fmt, ...){
	int n;
	va_list args;
	FILE *f=sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, 100000);
	va_start(args, fmt);
	n=vfprintf(f, fmt, args);
	va_end(args);
	sclose(f);
	return n;
}
