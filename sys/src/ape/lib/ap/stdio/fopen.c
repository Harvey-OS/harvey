/*
 * pANS stdio -- fopen
 */
#include "iolib.h"
FILE *fopen(const char *name, const char *mode){
	FILE *f;
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++)
		if(f->state==CLOSED)
			return freopen(name, mode, f);
	return NULL;
}
