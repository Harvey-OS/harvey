/*
 * pANS stdio -- scanf
 */
#include "iolib.h"
int scanf(const char *fmt, ...){
	int n;
	va_list args;
	va_start(args, fmt);
	n=vfscanf(stdin, fmt, args);
	va_end(args);
	return n;
}
