/*
 * pANS stdio -- fclose
 */
#include "iolib.h"
int fclose(FILE *f){
	int error=0;
	if(f->state==CLOSED) return EOF;
	if(fflush(f)==EOF) error=EOF;
	if(f->flags&BALLOC) free(f->buf);
	if(!(f->flags&STRING) && close(f->fd)<0) error=EOF;
	f->state=CLOSED;
	f->flags=0;
	return error;
}
