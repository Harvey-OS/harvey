/*
 * pANS stdio -- fputc
 */
#include "iolib.h"
int fputc(int c, FILE *f){
	return putc(c, f);	/* This can be made more fair to _IOLBF-mode streams */
}
