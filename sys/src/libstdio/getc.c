/*
 * pANS stdio -- getc
 */
#include "iolib.h"
#undef getc
int getc(FILE *f){
	return fgetc(f);
}
