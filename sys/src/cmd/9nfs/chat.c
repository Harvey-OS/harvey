/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

#define	SIZE	1024

int		chatty;
int		conftime;

#define	NSIZE	128

static int8_t	nbuf[NSIZE];
static int	chatpid;

static void
killchat(void)
{
	int8_t buf[NSIZE];
	int fd;

	remove(nbuf);
	snprint(buf, sizeof buf, "/proc/%d/note", chatpid);
	fd = open(buf, OWRITE);
	write(fd, "kill\n", 5);
	close(fd);
}

void
chatsrv(int8_t *name)
{
	int n, sfd, pfd[2];
	int8_t *p, buf[256];

	if(name && *name)
		snprint(nbuf, sizeof nbuf, "/srv/%s", name);
	else{
		if(p = strrchr(argv0, '/'))	/* assign = */
			name = p+1;
		else
			name = argv0;
		snprint(nbuf, sizeof nbuf, "/srv/%s.chat", name);
	}
	remove(nbuf);
	if(pipe(pfd) < 0)
		panic("chatsrv pipe");
	sfd = create(nbuf, OWRITE, 0600);
	if(sfd < 0)
		panic("chatsrv create %s", nbuf);
	chatpid = rfork(RFPROC|RFMEM);
	switch(chatpid){
	case -1:
		panic("chatsrv fork");
	case 0:
		break;
	default:
		atexit(killchat);
		return;
	}
	fprint(sfd, "%d", pfd[1]);
	close(sfd);
	close(pfd[1]);
	for(;;){
		n = read(pfd[0], buf, sizeof(buf)-1);
		if(n < 0)
			break;
		if(n == 0)
			continue;
		buf[n] = 0;
		if(buf[0] == 'c')
			conftime = 999;
		chatty = strtol(buf, 0, 0);
		if(abs(chatty) < 2)
			rpcdebug = 0;
		else
			rpcdebug = abs(chatty) - 1;
		fprint(2, "%s: chatty=%d, rpcdebug=%d, conftime=%d\n",
			nbuf, chatty, rpcdebug, conftime);
	}
	_exits(0);
}

void
chat(int8_t *fmt, ...)
{
	int8_t buf[SIZE];
	va_list arg;
	Fmt f;

	if(!chatty)
		return;

	fmtfdinit(&f, 2, buf, sizeof buf);
	va_start(arg, fmt);
	fmtvprint(&f, fmt, arg);
	va_end(arg);
	fmtfdflush(&f);
}

void
clog(int8_t *fmt, ...)
{
	int8_t buf[SIZE];
	va_list arg;
	int n;

	va_start(arg, fmt);
	vseprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	n = strlen(buf);
	if(chatty || rpcdebug)
		write(2, buf, n);
	if(chatty <= 0){
		if(n>0 && buf[n-1] == '\n')
			buf[n-1] = 0;
		syslog(0, "nfs", buf);
	}
}

void
panic(int8_t *fmt, ...)
{
	int8_t buf[SIZE];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	if(chatty || rpcdebug)
		fprint(2, "%s %d: %s: %r\n", argv0, getpid(), buf);
	if(chatty <= 0)
		syslog(0, "nfs", buf);
	exits("panic");
}
