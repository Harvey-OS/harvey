#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"


static long
t(Ioproc *io, void (*op)(Ioproc*), int n, ...)
{
	int i, ret;
	va_list arg;

	assert(!io->inuse);
	io->inuse = 1;
	io->op = op;
	va_start(arg, n);
	for(i=0; i<n; i++)
		io->arg[i] = va_arg(arg, long);
	sendp(io->c, io);
	recvp(io->c);
	ret = io->ret;
	if(ret < 0)
		errstr(io->err, sizeof io->err);
	io->inuse = 0;
	return ret;
}

static void
t2(Ioproc *io, int ret)
{
	io->ret = ret;
	if(ret < 0)
		rerrstr(io->err, sizeof io->err);
	sendp(io->c, io);
}

static void ioread2(Ioproc*);
static long
ioread(Ioproc *io, int fd, void *a, long n)
{
	return t(io, ioread2, 3, fd, a, n);
}
static void
ioread2(Ioproc *io)
{
	t2(io, read(io->arg[0], (void*)io->arg[1], io->arg[2]));
}

static void iowrite2(Ioproc*);
static long
iowrite(Ioproc *io, int fd, void *a, long n)
{
	return t(io, iowrite2, 3, fd, a, n);
}
static void
iowrite2(Ioproc *io)
{
	t2(io, write(io->arg[0], (void*)io->arg[1], io->arg[2]));
}

static void ioclose2(Ioproc*);
static int
ioclose(Ioproc *io, int fd)
{
	return t(io, ioclose2, 1, fd);
}
static void
ioclose2(Ioproc *io)
{
	t2(io, close(io->arg[0]));
}

static void
iointerrupt(Ioproc *io)
{
	if(!io->inuse)
		return;
	postnote(PNPROC, io->pid, "interrupt");
}

static int
pipenote(void*, char *msg)
{
	if(strstr(msg, "sys: write on closed pipe") || strstr(msg, "interrupt"))
		return 1;
	return 0;
}

static void
xioproc(void *a)
{
	Ioproc *io;

	threadnotify(pipenote, 1);
	io = a;
	io->pid = getpid();
	sendp(io->c, nil);
	for(;;){
		io = recvp(io->c);
		if(io == nil)
			continue;
		io->op(io);
	}
	chanfree(io->c);
	free(io);
}

Ioproc iofns =
{
	ioread,
	iowrite,
	ioclose,
	iointerrupt,
};

Ioproc *iofree;

Ioproc*
ioproc(void)
{
	Ioproc *io;

	if((io = iofree) != nil){
		iofree = io->next;
		return io;
	}
	io = emalloc(sizeof(*io));
	*io = iofns;
	io->c = chancreate(sizeof(void*), 0);
	if(proccreate(xioproc, io, STACK) < 0)
		sysfatal("proccreate: %r");
	recvp(io->c);
	return io;
}

void
closeioproc(Ioproc *io)
{
	io->next = iofree;
	iofree = io;
}
