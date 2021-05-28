/*
 * pANS stdio -- rewind
 */
#include "iolib.h"
void rewind(FILE *f){
	fseek(f, 0, SEEK_SET);
}
