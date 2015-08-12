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
#include <sunrpc.h>

enum {
	MaxRead = 17000,
};

typedef struct SunMsgFd SunMsgFd;
struct SunMsgFd {
	SunMsg msg;
	int fd;
};

typedef struct Arg Arg;
struct Arg {
	SunSrv *srv;
	Channel *creply;
	Channel *csync;
	int fd;
};

static void
sunFdRead(void *v)
{
	uint n, tot;
	int done;
	uint8_t buf[4], *p;
	Arg arg = *(Arg *)v;
	SunMsgFd *msg;

	sendp(arg.csync, 0);

	p = nil;
	tot = 0;
	for(;;) {
		n = readn(arg.fd, buf, 4);
		if(n != 4)
			break;
		n = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
		if(arg.srv->chatty)
			fprint(2, "%.8ux...", n);
		done = n & 0x80000000;
		n &= ~0x80000000;
		p = erealloc(p, tot + n);
		if(readn(arg.fd, p + tot, n) != n)
			break;
		tot += n;
		if(done) {
			msg = emalloc(sizeof(SunMsgFd));
			msg->msg.data = p;
			msg->msg.count = tot;
			msg->msg.creply = arg.creply;
			sendp(arg.srv->crequest, msg);
			p = nil;
			tot = 0;
		}
	}
}

static void
sunFdWrite(void *v)
{
	uint8_t buf[4];
	uint32_t n;
	Arg arg = *(Arg *)v;
	SunMsgFd *msg;

	sendp(arg.csync, 0);

	while((msg = recvp(arg.creply)) != nil) {
		n = msg->msg.count;
		buf[0] = (n >> 24) | 0x80;
		buf[1] = n >> 16;
		buf[2] = n >> 8;
		buf[3] = n;
		if(write(arg.fd, buf, 4) != 4 || write(arg.fd, msg->msg.data, msg->msg.count) != msg->msg.count)
			fprint(2, "sunFdWrite: %r\n");
		free(msg->msg.data);
		free(msg);
	}
}

int
sunSrvFd(SunSrv *srv, int fd)
{
	Arg *arg;

	arg = emalloc(sizeof(Arg));
	arg->fd = fd;
	arg->srv = srv;
	arg->csync = chancreate(sizeof(void *), 0);
	arg->creply = chancreate(sizeof(SunMsg *), 10);

	proccreate(sunFdRead, arg, SunStackSize);
	proccreate(sunFdWrite, arg, SunStackSize);
	recvp(arg->csync);
	recvp(arg->csync);

	chanfree(arg->csync);
	free(arg);
	return 0;
}
