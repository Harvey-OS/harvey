/*
 * pANS stdio -- setbuf
 */
#include "iolib.h"
void setbuf(FILE *f, char *buf){
	if(f->state==OPEN){
		if(buf)
			f->bufl=BUFSIZ;
		else{
			buf=f->unbuf;
			f->bufl=0;
		}
		f->rp=f->wp=f->lp=f->buf=buf;
		f->state=RDWR;
	}
	/* else error, but there's no way to report it */
}
