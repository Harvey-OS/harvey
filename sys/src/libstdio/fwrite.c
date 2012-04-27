/*
 * pANS stdio -- fwrite
 */
#include "iolib.h"

#define BIGN (BUFSIZ/2)

long fwrite(const void *p, long recl, long nrec, FILE *f){
	char *s;
	int n, d;

	s=(char *)p;
	n=recl*nrec;
	while(n>0){
		d=f->rp-f->wp;
		if(d>0){
			if(d>n)
				d=n;
			memmove(f->wp, s, d);
			f->wp+=d;
		}else{
			if(n>=BIGN && f->state==WR && !(f->flags&(STRING|LINEBUF)) && f->buf!=f->unbuf){
				d=f->wp-f->buf;
				if(d>0){
					if(f->flags&APPEND)
						seek(f->fd, 0L, 2);
					if(write(f->fd, f->buf, d)!=d){
						f->state=ERR;
						goto ret;
					}
					f->wp=f->rp=f->buf;
				}
				if(f->flags&APPEND)
					seek(f->fd, 0L, 2);
				d=write(f->fd, s, n);
				if(d<=0){
					f->state=ERR;
					goto ret;
				}
			}else{
				if(_IO_putc(*s, f)==EOF)
					goto ret;
				d=1;
			}
		}
		s+=d;
		n-=d;
	}
    ret:
	return (s-(char *)p)/(recl?recl:1);
}
