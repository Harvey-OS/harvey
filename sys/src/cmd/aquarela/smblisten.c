#include "headers.h"

static struct {
	int thread;
	QLock;
	char adir[NETPATHLEN];
	int acfd;
	char ldir[NETPATHLEN];
	int lcfd;
	SMBCIFSACCEPTFN *accept;
} tcp = { -1 };

typedef struct Session Session;

enum { Connected, Dead };

struct Session {
	SmbCifsSession;
	int thread;
	Session *next;
	int state;
	SMBCIFSWRITEFN *write;
};

static struct {
	QLock;
	Session *head;
} sessions;

typedef struct Listen Listen;

static void
deletesession(Session *s)
{
	Session **sp;
	close(s->fd);
	qlock(&sessions);
	for (sp = &sessions.head; *sp && *sp != s; sp = &(*sp)->next)
		;
	if (*sp)
		*sp = s->next;
	qunlock(&sessions);
	free(s);
}

static void
tcpreader(void *a)
{
	Session *s = a;
	uchar *buf;
	int buflen = smbglobals.maxreceive + 4;
	buf = nbemalloc(buflen);
	for (;;) {
		int n;
		uchar flags;
		ushort length;

		n = readn(s->fd, buf, 4);
		if (n != 4) {
		die:
			free(buf);
			if (s->state == Connected)
				(*s->write)(s, nil, -1);
			deletesession(s);
			return;
		}
		flags = buf[1];
		length = nhgets(buf + 2) | ((flags & 1) << 16);
		if (length > buflen - 4) {
			print("nbss: too much data (%ud)\n", length);
			goto die;
		}
		n = readn(s->fd, buf + 4, length);
		if (n != length)
			goto die;
		if (s->state == Connected) {
			if ((*s->write)(s, buf + 4, length) != 0) {
				s->state = Dead;
				goto die;
			}
		}
	}
}

static Session *
createsession(int fd)
{
	Session *s;
	s = smbemalloc(sizeof(Session));
	s->fd = fd;
	s->state = Connected;
	qlock(&sessions);
	if (!(*tcp.accept)(s, &s->write)) {
		qunlock(&sessions);
		free(s);
		return nil;
	}
	s->thread = procrfork(tcpreader, s, 32768, RFNAMEG);
	if (s->thread < 0) {
		qunlock(&sessions);
		(*s->write)(s, nil, -1);
		free(s);
		return nil;
	}
	s->next = sessions.head;
	sessions.head = s;
	qunlock(&sessions);
	return s;
}

static void
tcplistener(void *)
{
	for (;;) {
		int dfd;
		char ldir[NETPATHLEN];
		int lcfd;
//print("cifstcplistener: listening\n");
		lcfd = listen(tcp.adir, ldir);
//print("cifstcplistener: contact\n");
		if (lcfd < 0) {
		die:
			qlock(&tcp);
			close(tcp.acfd);
			tcp.thread = -1;
			qunlock(&tcp);
			return;
		}
		dfd = accept(lcfd, ldir);
		close(lcfd);
		if (dfd < 0)
			goto die;
		if (createsession(dfd) == nil)
			close(dfd);
	}
}

int
smblistencifs(SMBCIFSACCEPTFN *accept)
{
	qlock(&tcp);
	if (tcp.thread < 0) {
		tcp.acfd = announce("tcp!*!cifs", tcp.adir);
		if (tcp.acfd < 0) {
			print("smblistentcp: can't announce: %r\n");
			qunlock(&tcp);
			return -1;
		}
		tcp.thread = proccreate(tcplistener, nil, 16384);
	}
	tcp.accept = accept;
	qunlock(&tcp);
	return 0;
}
	