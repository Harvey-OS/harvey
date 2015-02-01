/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- rdline
 * This is not a pANS routine.
 */
#include "iolib.h"
#include <string.h>

char *rdline(FILE *f, char **ep){
	int cnt;
	char *nlp, *vp;
	switch(f->state){
	default:	/* CLOSED, WR, ERR, EOF */
		return NULL;
	case OPEN:
		_IO_setvbuf(f);
	case RDWR:
		f->state=RD;
	case RD:
		if(f->bufl==0){		/* Called by a comedian! */
			f->state=ERR;
			return NULL;
		}
		vp=f->rp;
		for(;;){
			/*
			 * Look for a newline.
			 * If none found, slide the partial line to the beginning
			 * of the buffer, read some more and keep looking.
			 */
			nlp=memchr(f->rp, '\n', f->wp-f->rp);
			if(nlp!=0) break;
			if(f->flags&STRING){
				f->rp=f->wp;
				if(ep) *ep=f->wp;
				return vp;
			}
			if(f->rp!=f->buf){
				memmove(f->buf, f->rp, f->wp-f->rp);
				f->wp-=f->rp-f->buf;
				f->rp=f->buf;
				vp=f->rp;
			}
			cnt=f->bufl-(f->wp-f->buf);
			if(cnt==0){	/* no room left */
				nlp=f->wp-1;
				break;
			}
			cnt=read(f->fd, f->wp, cnt);
			if(cnt==-1){
				f->state=ERR;
				return NULL;
			}
			if(cnt==0){	/* is this ok? */
				f->state=EOF;
				return NULL;
			}
			f->rp=f->wp;
			f->wp+=cnt;
		}
		*nlp='\0';
		f->rp=nlp+1;
		if(ep) *ep=nlp;
		return vp;
	}
}
