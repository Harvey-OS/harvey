/*
 * pANS stdio -- getchar
 */
#include "iolib.h"
#undef putchar
int putchar(int c){
	return fputc(c, stdout);
}
