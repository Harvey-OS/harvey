/*
 * pANS stdio -- fopen
 */
#include "iolib.h"
FILE *fopen(const char *name, const char *mode){
	FILE *f;
	qlock(&_stdiolk);
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++) {
		if(f->state==CLOSED) {
			qunlock(&_stdiolk);
			return freopen(name, mode, f);
		}
	}
	qunlock(&_stdiolk);
	return NULL;
}
