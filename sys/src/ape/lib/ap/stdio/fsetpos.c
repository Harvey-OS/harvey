/*
 * pANS stdio -- fsetpos
 */
#include "iolib.h"
int fsetpos(FILE *f, const fpos_t *pos){
	return fseek(f, *pos, SEEK_SET);
}
