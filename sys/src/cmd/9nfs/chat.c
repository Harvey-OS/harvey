#include "all.h"

#define	SIZE	1024

int		chatty;
int		conftime;

#define	NSIZE	128

static char	nbuf[NSIZE];
static int	chatpid;

static void
killchat(void)
{
	char buf[NSIZE];
	int fd;

	remove(nbuf);
	snprint(buf, sizeof buf, "/proc/%d/note", chatpid);
	fd = open(buf, OWRITE);
	write(fd, "kill\n", 5);
	close(fd);
}

void
chatsrv(char *name)
{
	int n, sfd, pfd[2];
	char *p, buf[256];

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
chat(char *fmt, ...)
{
	char buf[SIZE];
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
clog(char *fmt, ...)
{
	char buf[SIZE];
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
panic(char *fmt, ...)
{
	char buf[SIZE];
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
