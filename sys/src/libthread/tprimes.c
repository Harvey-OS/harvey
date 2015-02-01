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
#include <thread.h>

int quiet;
int goal;
int buffer;
int (*fn)(void(*)(void*), void*, uint) = threadcreate;

void
primethread(void *arg)
{
	Channel *c, *nc;
	int p, i;

	c = arg;
	p = recvul(c);
	if(p > goal)
		threadexitsall(nil);
	if(!quiet)
		print("%d\n", p);
	nc = chancreate(sizeof(ulong), buffer);
	(*fn)(primethread, nc, 1024);
	for(;;){
		i = recvul(c);
		if(i%p)
			sendul(nc, i);
	}
}

void
threadmain(int argc, char **argv)
{
	int i;
	Channel *c;

	ARGBEGIN{
	case 'q':
		quiet = 1;
		break;
	case 'b':
		buffer = atoi(ARGF());
		break;
	case 'p':
		fn=proccreate;
		break;
	}ARGEND

	if(argc>0)
		goal = atoi(argv[0]);
	else
		goal = 100;

	c = chancreate(sizeof(ulong), buffer);
	threadcreate(primethread, c, 1024);
	for(i=2;; i++)
		sendul(c, i);
}
