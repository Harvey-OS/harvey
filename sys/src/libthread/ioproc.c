#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

enum
{
	STACK = 8192,
};

static Ioproc *iofree;

void
iointerrupt(Ioproc *io)
{
	if(!io->inuse)
		return;
	postnote(PNPROC, io->pid, "threadint");
}

static void
xioproc(void *a)
{
	Ioproc *io;

	io = a;
	io->pid = getpid();
	sendp(io->c, nil);
	while(recvp(io->c) == io){
		io->ret = io->op(&io->arg);
		if(io->ret < 0)
			rerrstr(io->err, sizeof io->err);
		sendp(io->c, io);
	}
	chanfree(io->c);
	free(io);
}

Ioproc*
ioproc(void)
{
	Ioproc *io;

	if((io = iofree) != nil){
		iofree = io->next;
		return io;
	}
	io = mallocz(sizeof(*io), 1);
	if(io == nil)
		sysfatal("ioproc malloc: %r");
	io->c = chancreate(sizeof(void*), 0);
	if(proccreate(xioproc, io, STACK) < 0)
		sysfatal("ioproc proccreate: %r");
	recvp(io->c);
	return io;
}

void
closeioproc(Ioproc *io)
{
	if(io == nil)
		return;
	iointerrupt(io);
	io->next = iofree;
	iofree = io;
}
