/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- ungetc
 */
#include "iolib.h"
int ungetc(int c, FILE *f){
	if(c==EOF) return EOF;
	switch(f->state){
	default:	/* WR */
		f->state=ERR;
		return EOF;
	case CLOSED:
	case ERR:
		return EOF;
	case OPEN:
		_IO_setvbuf(f);
	case RDWR:
	case END:
		f->rp=f->wp=f->buf+(f->bufl==0?1:f->bufl);
		f->state=RD;
	case RD:
		if(f->rp==f->buf) return EOF;
		if(f->flags&STRING)
			f->rp--;
		else
			*--f->rp=c;
		return (char)c;
	}
}
