/*
 * pANS stdio -- fread
 */
#include "iolib.h"

#define BIGN (BUFSIZ/2)

long fread(void *p, long recl, long nrec, FILE *f){
	char *s;
	int n, d, c;

	s=(char *)p;
	n=recl*nrec;
	while(n>0){
		d=f->wp-f->rp;
		if(d>0){
			if(d>n)
				d=n;
			memmove(s, f->rp, d);
			f->rp+=d;
		}else{
			if(n >= BIGN && f->state==RD && !(f->flags&STRING) && f->buf!=f->unbuf){
				d=read(f->fd, s, n);
				if(d<=0){
					f->state=(d==0)?END:ERR;
					goto ret;
				}
			}else{
 				c=_IO_getc(f);
				if(c==EOF)
					goto ret;
				*s=c;
				d=1;
			}
		}
		s+=d;
		n-=d;
	}
    ret:
	return (s-(char *)p)/(recl?recl:1);
}
