/*
 * pANS stdio -- sopenw
 */
#include "iolib.h"

FILE *_IO_sopenw(void){
	FILE *f;
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++) if(f->state==CLOSED) break;
	if(f==&_IO_stream[FOPEN_MAX]) return NULL;
	f->buf=f->rp=f->wp=0;
	f->state=OPEN;
	f->flags=STRING;
	f->fd=-1;
	return f;
}
