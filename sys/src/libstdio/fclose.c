/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- fclose
 */
#include "iolib.h"
int fclose(FILE *f){
	int error=0;
	if(f->state==CLOSED) return EOF;
	if(fflush(f)==EOF) error=EOF;
	if(f->flags&BALLOC) free(f->buf);
	if(!(f->flags&STRING) && close(f->fd)<0) error=EOF;
	f->state=CLOSED;
	f->flags=0;
	return error;
}
