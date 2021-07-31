#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

long
iocall(Ioproc *io, long (*op)(va_list*), ...)
{
	int ret;

	assert(!io->inuse);
	io->inuse = 1;
	io->op = op;
	va_start(io->arg, op);
	sendp(io->c, io);
	recvp(io->c);
	va_end(io->arg);
	ret = io->ret;
	if(ret < 0)
		errstr(io->err, sizeof io->err);
	io->inuse = 0;
	return ret;
}
