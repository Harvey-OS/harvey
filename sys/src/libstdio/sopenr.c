/*
 * pANS stdio -- sopenr
 */
#include "iolib.h"

FILE *sopenr(const char *s){
	FILE *f;
	qlock(&_stdiolk);
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++) if(f->state==CLOSED) break;
	if(f==&_IO_stream[FOPEN_MAX]) {
		qunlock(&_stdiolk);
		return NULL;
	}
	f->buf=f->rp=(char *)s;	/* what an annoyance const is */
	f->bufl=strlen(s);
	f->wp=f->buf+f->bufl;
	f->state=RD;
	f->flags=STRING;
	f->fd=-1;
	qunlock(&_stdiolk);
	return f;
}
