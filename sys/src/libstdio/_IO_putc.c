/*
 * pANS stdio -- _IO_putc, _IO_cleanup
 */
#include "iolib.h"
void _IO_cleanup(void){
	fflush(NULL);
}
/*
 * Look this over for simplification
 */
int _IO_putc(int c, FILE *f){
	int cnt;
	static int first=1;
	switch(f->state){
	case RD:
		f->state=ERR;
	case ERR:
	case CLOSED:
		return EOF;
	case OPEN:
		_IO_setvbuf(f);
		/* fall through */
	case RDWR:
	case END:
		f->rp=f->buf+f->bufl;
		if(f->flags&LINEBUF){
			f->wp=f->rp;
			f->lp=f->buf;
		}
		else
			f->wp=f->buf;
		break;
	}
	if(first){
		atexit(_IO_cleanup);
		first=0;
	}
	if(f->flags&STRING){
		f->rp=f->buf+f->bufl;
		if(f->wp==f->rp){
			if(f->flags&BALLOC)
				f->buf=realloc(f->buf, f->bufl+BUFSIZ);
			else{
				f->state=ERR;
				return EOF;
			}
			if(f->buf==NULL){
				f->state=ERR;
				return EOF;
			}
			f->rp=f->buf+f->bufl;
			f->bufl+=BUFSIZ;
		}
		*f->wp++=c;
	}
	else if(f->flags&LINEBUF){
		if(f->lp==f->rp){
			cnt=f->lp-f->buf;
			if(f->flags&APPEND) seek(f->fd, 0L, 2);
			if(cnt!=0 && write(f->fd, f->buf, cnt)!=cnt){
				f->state=ERR;
				return EOF;
			}
			f->lp=f->buf;
		}
		*f->lp++=c;
		if(c=='\n'){
			cnt=f->lp-f->buf;
			if(f->flags&APPEND) seek(f->fd, 0L, 2);
			if(cnt!=0 && write(f->fd, f->buf, cnt)!=cnt){
				f->state=ERR;
				return EOF;
			}
			f->lp=f->buf;
		}
	}
	else if(f->buf==f->unbuf){
		f->unbuf[0]=c;
		if(f->flags&APPEND) seek(f->fd, 0L, 2);
		if(write(f->fd, f->buf, 1)!=1){
			f->state=ERR;
			return EOF;
		}
	}
	else{
		if(f->wp==f->rp){
			cnt=f->wp-f->buf;
			if(f->flags&APPEND) seek(f->fd, 0L, 2);
			if(cnt!=0 && write(f->fd, f->buf, cnt)!=cnt){
				f->state=ERR;
				return EOF;
			}
			f->wp=f->buf;
			f->rp=f->buf+f->bufl;
		}
		*f->wp++=c;
	}
	f->state=WR;
	/*
	 * Make sure EOF looks different from putc(-1)
	 * Should be able to cast to unsigned char, but
	 * there's a vc bug preventing that from working
	 */
	return c&0xff;
}
