/*
 * pANS stdio -- ungetc
 */
#include "iolib.h"
int ungetc(int c, FILE *f){
	if(c==EOF) return EOF;
	switch(f->state){
	default:	/* WR */
		f->state=ERR;
		return EOF;
	case CLOSED:
	case ERR:
		return EOF;
	case OPEN:
		_IO_setvbuf(f);
	case RDWR:
	case END:
		f->rp=f->wp=f->buf+(f->bufl==0?1:f->bufl);
		f->state=RD;
	case RD:
		if(f->rp==f->buf) return EOF;
		if(f->flags&STRING)
			f->rp--;
		else
			*--f->rp=c;
		return (char)c;
	}
}
