/*
 * pANS stdio -- ferror
 */
#include "iolib.h"
int ferror(FILE *f){
	return f->state==ERR;
}
