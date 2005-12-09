/*
 * pANS stdio -- fread
 */
#include "iolib.h"
#include <string.h>

#define BIGN (BUFSIZ/2)

size_t fread(void *p, size_t recl, size_t nrec, FILE *f){
	char *s;
	int n, d, c;

	s=(char *)p;
	n=recl*nrec;
	while(n>0){
		d=f->wp-f->rp;
		if(d>0){
			if(d>n)
				d=n;
			memcpy(s, f->rp, d);
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
	if(recl)
		return (s-(char*)p)/recl;
	else
		return s-(char*)p;
}
