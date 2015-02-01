/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- sopenr
 */
#include "iolib.h"
#include <string.h>

FILE *_IO_sopenr(const char *s){
	FILE *f;
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++) if(f->state==CLOSED) break;
	if(f==&_IO_stream[FOPEN_MAX]) return NULL;
	f->buf=f->rp=(char *)s;	/* what an annoyance const is */
	f->bufl=strlen(s);
	f->wp=f->buf+f->bufl;
	f->state=RD;
	f->flags=STRING;
	f->fd=-1;
	return f;
}
