/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
