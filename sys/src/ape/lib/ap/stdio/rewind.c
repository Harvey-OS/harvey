/*
 * pANS stdio -- rewind
 */
#include "iolib.h"
void rewind(FILE *f){
	fseek(f, 0L, SEEK_SET);
}
