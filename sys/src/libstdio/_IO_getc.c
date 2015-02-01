/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- _IO_getc
 */
#include "iolib.h"
int _IO_getc(FILE *f){
	int cnt;
	switch(f->state){
	default:	/* CLOSED, WR, ERR, EOF */
		return EOF;
	case OPEN:
		_IO_setvbuf(f);
	case RDWR:
	case RD:
		if(f->flags&STRING) return EOF;
		cnt=read(f->fd, f->buf, f->buf==f->unbuf?1:f->bufl);
		switch(cnt){
		case -1: f->state=ERR; return EOF;
		case 0: f->state=END; return EOF;
		default:
			f->state=RD;
			f->rp=f->buf;
			f->wp=f->buf+cnt;
			return (*f->rp++)&_IO_CHMASK;
		}
	}
}
