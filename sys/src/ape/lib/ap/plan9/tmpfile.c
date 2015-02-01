/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sys9.h"
#undef OPEN
#include "../stdio/iolib.h"
#include "lib.h"
#include "dir.h"

FILE *
tmpfile(void){
	FILE *f;
	static char name[]="/tmp/tf0000000000000";
	char *p;
	int n;
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++)
		if(f->state==CLOSED)
			break;
	if(f==&_IO_stream[FOPEN_MAX])
		return NULL;
	while(access(name, 0) >= 0){
		p = name+7;
		while(*p == '9')
			*p++ = '0';
		if(*p == '\0')
			return NULL;
		++*p;
	}
	n = _CREATE(name, 64|2, 0777); /* remove-on-close */
	if(n==-1){
		_syserrno();
		return NULL;
	}
	_fdinfo[n].flags = FD_ISOPEN;
	_fdinfo[n].oflags = 2;
	f->fd=n;
	f->flags=0;
	f->state=OPEN;
	f->buf=0;
	f->rp=0;
	f->wp=0;
	f->lp=0;
	return f;
}
