/*
 * pANS stdio -- fgetpos
 */
#include "iolib.h"
int fgetpos(FILE *f, fpos_t *pos){
	*pos=ftell(f);
	return *pos==-1?-1:0;
}
