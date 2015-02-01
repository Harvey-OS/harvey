/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/times.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static
char*
skip(char *p)
{

	while(*p == ' ')
		p++;
	while(*p != ' ' && *p != 0)
		p++;
	return p;
}

clock_t
times(struct tms *buf)
{
	char b[200], *p;
	int f;
	unsigned long r;

	memset(b, 0, sizeof(b));
	f = open("/dev/cputime", O_RDONLY);
	if(f >= 0) {
		lseek(f, SEEK_SET, 0);
		read(f, b, sizeof(b));
		close(f);
	}
	p = b;
	if(buf)
		buf->tms_utime = atol(p);
	p = skip(p);
	if(buf)
		buf->tms_stime = atol(p);
	p = skip(p);
	r = atol(p);
	if(buf){
		p = skip(p);
		buf->tms_cutime = atol(p);
		p = skip(p);
		buf->tms_cstime = atol(p);
	}
	return r;
}
