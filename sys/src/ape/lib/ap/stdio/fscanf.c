/*
 * pANS stdio -- fscanf
 */
#include "iolib.h"
int fscanf(FILE *f, const char *fmt, ...){
	int n;
	va_list args;
	va_start(args, fmt);
	n=vfscanf(f, fmt, args);
	va_end(args);
	return n;
}
