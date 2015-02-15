/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

#define	NFN	33
static	int	(*onnot[NFN])(void*, char*);
static	Lock	onnotlock;

static
void
notifier(void *v, char *s)
{
	int i;

	for(i=0; i<NFN; i++)
		if(onnot[i] && ((*onnot[i])(v, s))){
			noted(NCONT);
			return;
		}
	noted(NDFLT);
}

int
atnotify(int (*f)(void*, char*), int in)
{
	int i, n, ret;
	static int init;

	if(!init){
		notify(notifier);
		init = 1;		/* assign = */
	}
	ret = 0;
	lock(&onnotlock);
	if(in){
		for(i=0; i<NFN; i++)
			if(onnot[i] == 0) {
				onnot[i] = f;
				ret = 1;
				break;
			}
	}else{
		n = 0;
		for(i=0; i<NFN; i++)
			if(onnot[i]){
				if(ret==0 && onnot[i]==f){
					onnot[i] = 0;
					ret = 1;
				}else
					n++;
			}
		if(n == 0){
			init = 0;
			notify(0);
		}
	}
	unlock(&onnotlock);
	return ret;
}
