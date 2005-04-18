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
		return setvbuf(f, (char *)0, _IOLBF, BUFSIZ);
	else return setvbuf(f, (char *)0, _IOFBF, BUFSIZ);
}
static int
isatty(int fd)
{
	char buf[64];

	if(fd2path(fd, buf, sizeof buf) != 0)
		return 0;

	/* might be /mnt/term/dev/cons */
	return strlen(buf) >= 9 && strcmp(buf+strlen(buf)-9, "/dev/cons") == 0;
}
