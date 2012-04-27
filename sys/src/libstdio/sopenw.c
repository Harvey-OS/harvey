/*
 * pANS stdio -- sopenw
 */
#include "iolib.h"
FILE *sopenw(void){
	FILE *f;
	qlock(&_stdiolk);
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++) if(f->state==CLOSED) break;
	if(f==&_IO_stream[FOPEN_MAX]) {
		qunlock(&_stdiolk);
		return NULL;
	}
	f->buf=f->rp=f->wp=0;
	f->state=OPEN;
	f->flags=STRING;
	f->fd=-1;
	qunlock(&_stdiolk);
	return f;
}
