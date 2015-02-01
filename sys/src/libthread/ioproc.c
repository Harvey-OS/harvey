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
#include "threadimpl.h"

enum
{
	STACK = 8192,
};

void
iointerrupt(Ioproc *io)
{
	if(!io->inuse)
		return;
	threadint(io->tid);
}

static void
xioproc(void *a)
{
	Ioproc *io, *x;
	io = a;
	/*
	 * first recvp acquires the ioproc.
	 * second tells us that the data is ready.
	 */
	for(;;){
		while(recv(io->c, &x) == -1)
			;
		if(x == 0)	/* our cue to leave */
			break;
		assert(x == io);

		/* caller is now committed -- even if interrupted he'll return */
		while(recv(io->creply, &x) == -1)
			;
		if(x == 0)	/* caller backed out */
			continue;
		assert(x == io);

		io->ret = io->op(&io->arg);
		if(io->ret < 0)
			rerrstr(io->err, sizeof io->err);
		while(send(io->creply, &io) == -1)
			;
		while(recv(io->creply, &x) == -1)
			;
	}
}

Ioproc*
ioproc(void)
{
	Ioproc *io;

	io = mallocz(sizeof(*io), 1);
	if(io == nil)
		sysfatal("ioproc malloc: %r");
	io->c = chancreate(sizeof(void*), 0);
	io->creply = chancreate(sizeof(void*), 0);
	io->tid = proccreate(xioproc, io, STACK);
	return io;
}

void
closeioproc(Ioproc *io)
{
	if(io == nil)
		return;
	iointerrupt(io);
	while(send(io->c, 0) == -1)
		;
	chanfree(io->c);
	chanfree(io->creply);
	free(io);
}
