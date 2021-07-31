/*
 * pANS stdio -- sscanf
 */
#include "iolib.h"
int sscanf(const char *s, const char *fmt, ...){
	int n;
	FILE *f=sopenr(s);
	va_list args;
	va_start(args, fmt);
	n=vfscanf(f, fmt, args);
	va_end(args);
	sclose(f);
	return n;
}
