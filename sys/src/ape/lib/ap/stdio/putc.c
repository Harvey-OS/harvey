/*
 * pANS stdio -- putc
 */
#include "iolib.h"
#undef putc
int putc(int c, FILE *f){
	return fputc(c, f);
}
