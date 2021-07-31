/*
 * pANS stdio -- _IO_getc
 */
#include "iolib.h"
int _IO_getc(FILE *f){
	int cnt;
	switch(f->state){
	default:	/* CLOSED, WR, ERR, EOF */
		return EOF;
	case OPEN:
		_IO_setvbuf(f);
	case RDWR:
	case RD:
		if(f->flags&STRING) return EOF;
		cnt=read(f->fd, f->buf, f->buf==f->unbuf?1:f->bufl);
		switch(cnt){
		case -1: f->state=ERR; return EOF;
		case 0: f->state=END; return EOF;
		default:
			f->state=RD;
			f->rp=f->buf;
			f->wp=f->buf+cnt;
			return (*f->rp++)&_IO_CHMASK;
		}
	}
}
