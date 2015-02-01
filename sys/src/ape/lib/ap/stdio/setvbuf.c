/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- setvbuf
 */
#include "iolib.h"
#include <stdlib.h>
int setvbuf(FILE *f, char *buf, int mode, size_t size){
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
	if(f==stderr || (f==stdout && isatty(1)))
		return setvbuf(f, (char *)0, _IOLBF, BUFSIZ);
	return setvbuf(f, (char *)0, _IOFBF, BUFSIZ);
}
