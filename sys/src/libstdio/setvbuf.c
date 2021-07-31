/*
 * pANS stdio -- setvbuf
 */
#include "iolib.h"
int setvbuf(FILE *f, char *buf, int mode, long size){
	if(f->state!=OPEN){
		f->state=ERR;
		return -1;
	}
	f->state=RDWR;
	switch(mode){
	case _IOLBF:
		f->flags|=LINEBUF;
	case _IOFBF:
		if(buf==0){
			buf=malloc(size);
			if(buf==0){
				f->state=ERR;
				return -1;
			}
			f->flags|=BALLOC;
		}
		f->bufl=size;
		break;
	case _IONBF:
		buf=f->unbuf;
		f->bufl=0;
		break;
	}
	f->rp=f->wp=f->lp=f->buf=buf;
	f->state=RDWR;
	return 0;
}
int _IO_setvbuf(FILE *f){
	static int isatty(int);
	if(f==stderr || (f==stdout && isatty(1)))
		setvbuf(f, (char *)0, _IOLBF, BUFSIZ);
	else setvbuf(f, (char *)0, _IOFBF, BUFSIZ);
}
static int
isatty(int fd)
{
	Dir *d1, *d2;
	int ret;

	d1 = dirfstat(fd);
	d2 = dirstat("/dev/cons");
	ret = 0;
	if(d1!=nil && d2!=nil)
		ret = (d1->qid.path == d2->qid.path) &&
			(d1->type == d2->type) &&
			(d1->dev == d2->dev);
	free(d1);
	free(d2);
	return 0;
}
