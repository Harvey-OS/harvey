/*
 * pANS stdio -- fputs
 */
#include "iolib.h"
int fputs(const char *s, FILE *f){
	while(*s) putc(*s++, f);
	return ferror(f)?EOF:0;
}
