#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include <mp.h>
#include <libsec.h>
#include "dat.h"
#include "fns.h"

static int tlsdial(char*, char*, char*, int*, int);

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

static void iodial2(Ioproc*);
static int
iodial(Ioproc *io, char *a, char *b, char *c, int *d, int e)
{
	return t(io, iodial2, 5, a, b, c, d, e);
}
static void
iodial2(Ioproc *io)
{
	t2(io, tlsdial((char*)io->arg[0], (char*)io->arg[1], (char*)io->arg[2], (int*)io->arg[3], io->arg[4]));
}

static void ioopen2(Ioproc*);
static int
ioopen(Ioproc *io, char *path, int mode)
{
	return t(io, ioopen2, 2, path, mode);
}
static void
ioopen2(Ioproc *io)
{
	t2(io, open((char*)io->arg[0], io->arg[1]));
}

static int
ioprint(Ioproc *io, int fd, char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);
	return iowrite(io, fd, buf, strlen(buf));
}

static void
iointerrupt(Ioproc *io)
{
	if(!io->inuse)
		return;
	postnote(PNPROC, io->pid, "interrupt");
}

static void
xioproc(void *a)
{
	Ioproc *io;

	io = a;
	io->pid = getpid();
	sendp(io->c, nil);
	while(recvp(io->c) == io)
		io->op(io);
	chanfree(io->c);
	free(io);
}

static int
tlsdial(char *a, char *b, char *c, int *d, int usetls)
{
	int fd, tfd;
	TLSconn conn;

	fd = dial(a, b, c, d);
	if(fd < 0)
		return -1;
	if(usetls == 0)
		return fd;
	
	memset(&conn, 0, sizeof conn);
	tfd = tlsClient(fd, &conn);
	if(tfd < 0){
		print("tls %r\n");
		close(fd);
		return -1;
	}
	/* BUG: check cert here? */
	if(conn.cert)
		free(conn.cert);
	close(fd);
	return tfd;
}

Ioproc iofns =
{
	ioread,
	iowrite,
	iodial,
	ioclose,
	ioopen,
	ioprint,
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

