/*
 * pANS stdio -- printf
 */
#include "iolib.h"
int printf(const char *fmt, ...){
	int n;
	va_list args;
	va_start(args, fmt);
	n=vfprintf(stdout, fmt, args);
	va_end(args);
	return n;
}
