/*
 * pANS stdio -- sclose
 */
#include "iolib.h"
char *sclose(FILE *f){
	switch(f->state){
	default:	/* ERR CLOSED */
		if(f->buf && f->flags&BALLOC)
			free(f->buf);
		f->flags=0;
		return NULL;
	case OPEN:
		f->buf=malloc(1);
		f->buf[0]='\0';
		break;
	case RD:
	case END:
		f->flags=0;
		break;
	case RDWR:
	case WR:
		if(f->wp==f->rp){
			if(f->flags&BALLOC)
				f->buf=realloc(f->buf, f->bufl+1);
			if(f->buf==NULL) return NULL;
		}
		*f->wp='\0';
		f->flags=0;
		break;
	}
	f->state=CLOSED;
	f->flags=0;
	return f->buf;
}
