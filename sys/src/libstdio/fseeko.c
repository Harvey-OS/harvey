/*
 * pANS stdio -- fseeko
 */
#include "iolib.h"
int fseeko(FILE *f, long long offs, int type){
	switch(f->state){
	case ERR:
	case CLOSED:
		return -1;
	case WR:
		fflush(f);
		break;
	case RD:
		if(type==1 && f->buf!=f->unbuf)
			offs-=f->wp-f->rp;
		break;
	}
	if(f->flags&STRING || seek(f->fd, offs, type)==-1) return -1;
	if(f->state==RD) f->rp=f->wp=f->buf;
	if(f->state!=OPEN)
		f->state=RDWR;
	return 0;
}
