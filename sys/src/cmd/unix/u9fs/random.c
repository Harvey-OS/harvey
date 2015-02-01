/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>

static long
getseed(void)
{
	struct timeval tv;
	long seed;
	int fd, len;

	len = 0;
	fd = open("/dev/urandom", O_RDONLY);
	if(fd > 0){
		len = readn(fd, &seed, sizeof(seed));
		close(fd);
	}
	if(len != sizeof(seed)){
		gettimeofday(&tv, nil);
		seed = tv.tv_sec ^ tv.tv_usec ^ (getpid()<<8);
	}
	return seed;
}

static int seeded;

void
randombytes(uchar *r, uint nr)
{
	int i;
	ulong l;

	if(!seeded){
		seeded=1;
		srandom(getseed());
	}
	for(i=0; i+4<=nr; i+=4,r+=4){
		l = (ulong)random();
		r[0] = l;
		r[1] = l>>8;
		r[2] = l>>16;
		r[3] = l>>24;
	}
	if(i<nr){
		l = (ulong)random();
		switch(nr-i){
		case 3:
			r[2] = l>>16;
		case 2:
			r[1] = l>>8;
		case 1:
			r[0] = l;
		}
	}
}
