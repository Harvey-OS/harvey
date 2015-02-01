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

long
iocall(Ioproc *io, long (*op)(va_list*), ...)
{
	int ret, inted;
	Ioproc *msg;

	if(send(io->c, &io) == -1){
		werrstr("interrupted");
		return -1;
	}
	assert(!io->inuse);
	io->inuse = 1;
	io->op = op;
	va_start(io->arg, op);
	msg = io;
	inted = 0;
	while(send(io->creply, &msg) == -1){
		msg = nil;
		inted = 1;
	}
	if(inted){
		werrstr("interrupted");
		return -1;
	}

	/*
	 * If we get interrupted, we have to stick around so that
	 * the IO proc has someone to talk to.  Send it an interrupt
	 * and try again.
	 */
	inted = 0;
	while(recv(io->creply, nil) == -1){
		inted = 1;
		iointerrupt(io);
	}
	USED(inted);
	va_end(io->arg);
	ret = io->ret;
	if(ret < 0)
		errstr(io->err, sizeof io->err);
	io->inuse = 0;

	/* release resources */
	while(send(io->creply, &io) == -1)
		;
	return ret;
}
