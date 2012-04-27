#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static struct {
	int thread;
	QLock;
	char adir[NETPATHLEN];
	int acfd;
	char ldir[NETPATHLEN];
	int lcfd;
} tcp = { -1 };

typedef struct Session Session;

enum { NeedSessionRequest, Connected, Dead };

struct Session {
	NbSession;
	int thread;
	Session *next;
	int state;
	NBSSWRITEFN *write;
};

static struct {
	QLock;
	Session *head;
} sessions;

typedef struct Listen Listen;

struct Listen {
	NbName to;
	NbName from;
	int (*accept)(void *magic, NbSession *s, NBSSWRITEFN **writep);
	void *magic;
	Listen *next;
};

static struct {
	QLock;
	Listen *head;
} listens;

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
	int buflen = 0x1ffff + 4;
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
		n = readn(s->fd, buf + 4, length);
		if (n != length)
			goto die;
		if (flags & 0xfe) {
			print("nbss: invalid flags field 0x%.2ux\n", flags);
			goto die;
		}
		switch (buf[0]) {
		case 0: /* session message */
			if (s->state != Connected && s->state != Dead) {
				print("nbss: unexpected session message\n");
				goto die;
			}
			if (s->state == Connected) {
				if ((*s->write)(s, buf + 4, length) != 0) {
					s->state = Dead;
					goto die;
				}
			}
			break;
		case 0x81: /* session request */ {
			uchar *p, *ep;
			Listen *l;
			int k;
			int called_found;
			uchar error_code;

			if (s->state == Connected) {
				print("nbss: unexpected session request\n");
				goto die;
			}
			p = buf + 4;
			ep = p + length;
			k = nbnamedecode(p, p, ep, s->to);
			if (k == 0) {
				print("nbss: malformed called name in session request\n");
				goto die;
			}
			p += k;
			k = nbnamedecode(p, p, ep, s->from);
			if (k == 0) {
				print("nbss: malformed calling name in session request\n");
				goto die;
			}
/*
			p += k;
			if (p != ep) {
				print("nbss: extra data at end of session request\n");
				goto die;
			}
*/
			called_found = 0;
//print("nbss: called %B calling %B\n", s->to, s->from);
			qlock(&listens);
			for (l = listens.head; l; l = l->next)
				if (nbnameequal(l->to, s->to)) {
					called_found = 1;
					if (nbnameequal(l->from, s->from))
						break;
				}
			if (l == nil) {
				qunlock(&listens);
				error_code  = called_found ? 0x81 : 0x80;
			replydie:
				buf[0] = 0x83;
				buf[1] = 0;
				hnputs(buf + 2, 1);
				buf[4] = error_code;
				write(s->fd, buf, 5);
				goto die;
			}
			if (!(*l->accept)(l->magic, s, &s->write)) {
				qunlock(&listens);
				error_code = 0x83;
				goto replydie;
			}
			buf[0] = 0x82;
			buf[1] = 0;
			hnputs(buf + 2, 0);
			if (write(s->fd, buf, 4) != 4) {
				qunlock(&listens);
				goto die;
			}
			s->state = Connected;
			qunlock(&listens);
			break;
		}
		case 0x85: /* keep awake */
			break;
		default:
			print("nbss: opcode 0x%.2ux unexpected\n", buf[0]);
			goto die;
		}
	}
}

static NbSession *
createsession(int fd)
{
	Session *s;
	s = nbemalloc(sizeof(Session));
	s->fd = fd;
	s->state = NeedSessionRequest;
	qlock(&sessions);
	s->thread = procrfork(tcpreader, s, 32768, RFNAMEG);
	if (s->thread < 0) {
		qunlock(&sessions);
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
//print("tcplistener: listening\n");
		lcfd = listen(tcp.adir, ldir);
//print("tcplistener: contact\n");
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
nbsslisten(NbName to, NbName from,int (*accept)(void *magic, NbSession *s, NBSSWRITEFN **writep), void *magic)
{
	Listen *l;
	qlock(&tcp);
	if (tcp.thread < 0) {
		fmtinstall('B', nbnamefmt);
		tcp.acfd = announce("tcp!*!netbios", tcp.adir);
		if (tcp.acfd < 0) {
			print("nbsslisten: can't announce: %r\n");
			qunlock(&tcp);
			return -1;
		}
		tcp.thread = proccreate(tcplistener, nil, 16384);
	}
	qunlock(&tcp);
	l = nbemalloc(sizeof(Listen));
	nbnamecpy(l->to, to);
	nbnamecpy(l->from, from);
	l->accept = accept;
	l->magic = magic;
	qlock(&listens);
	l->next = listens.head;
	listens.head = l;
	qunlock(&listens);
	return 0;
}

void
nbssfree(NbSession *s)
{	
	deletesession((Session *)s);
}

int
nbssgatherwrite(NbSession *s, NbScatterGather *a)
{
	uchar hdr[4];
	NbScatterGather *ap;
	long l = 0;
	for (ap = a; ap->p; ap++)
		l += ap->l;
//print("nbssgatherwrite %ld bytes\n", l);
	hnputl(hdr, l);
//nbdumpdata(hdr, sizeof(hdr));
	if (write(s->fd, hdr, sizeof(hdr)) != sizeof(hdr))
		return -1;
	for (ap = a; ap->p; ap++) {
//nbdumpdata(ap->p, ap->l);
		if (write(s->fd, ap->p, ap->l) != ap->l)
			return -1;
	}
	return 0;
}

NbSession *
nbssconnect(NbName to, NbName from)
{
	Session *s;
	uchar ipaddr[IPaddrlen];
	char dialaddress[100];
	char dir[NETPATHLEN];
	uchar msg[576];
	int fd;
	long o;
	uchar flags;
	long length;

	if (!nbnameresolve(to, ipaddr))
		return nil;
	fmtinstall('I', eipfmt);
	snprint(dialaddress, sizeof(dialaddress), "tcp!%I!netbios", ipaddr);
	fd = dial(dialaddress, nil, dir, nil);
	if (fd < 0)
		return nil;
	msg[0] = 0x81;
	msg[1] = 0;
	o = 4;
	o += nbnameencode(msg + o, msg + sizeof(msg) - o, to);
	o += nbnameencode(msg + o, msg + sizeof(msg) - o, from);
	hnputs(msg + 2, o - 4);
	if (write(fd, msg, o) != o) {
		close(fd);
		return nil;
	}
	if (readn(fd, msg, 4) != 4) {
		close(fd);
		return nil;
	}
	flags = msg[1];
	length = nhgets(msg + 2) | ((flags & 1) << 16);
	switch (msg[0]) {
	default:
		close(fd);
		werrstr("unexpected session message code 0x%.2ux", msg[0]);
		return nil;
	case 0x82:
		if (length != 0) {
			close(fd);
			werrstr("length not 0 in positive session response");
			return nil;
		}
		break;
	case 0x83:
		if (length != 1) {
			close(fd);
			werrstr("length not 1 in negative session response");
			return nil;
		}
		if (readn(fd, msg + 4, 1) != 1) {
			close(fd);
			return nil;
		}
		close(fd);
		werrstr("negative session response 0x%.2ux", msg[4]);
		return nil;
	}
	s = nbemalloc(sizeof(Session));
	s->fd = fd;
	s->state = Connected;
	qlock(&sessions);
	s->next = sessions.head;
	sessions.head = s;
	qunlock(&sessions);
	return s;
}

long
nbssscatterread(NbSession *nbs, NbScatterGather *a)
{
	uchar hdr[4];
	uchar flags;
	long length, total;
	NbScatterGather *ap;
	Session *s = (Session *)nbs;

	long l = 0;
	for (ap = a; ap->p; ap++)
		l += ap->l;
//print("nbssscatterread %ld bytes\n", l);
again:
	if (readn(s->fd, hdr, 4) != 4) {
	dead:
		s->state = Dead;
		return -1;
	}
	flags = hdr[1];
	length = nhgets(hdr + 2) | ((flags & 1) << 16);
//print("%.2ux: %d\n", hdr[0], length);
	switch (hdr[0]) {
	case 0x85:
		if (length != 0) {
			werrstr("length in keepalive not 0");
			goto dead;
		}
		goto again;
	case 0x00:
		break;
	default:
		werrstr("unexpected session message code 0x%.2ux", hdr[0]);
		goto dead;
	}
	if (length > l) {
		werrstr("message too big (%ld)", length);
		goto dead;
	}
	total = length;
	for (ap = a; length && ap->p; ap++) {
		long thistime;
		long n;
		thistime = length;
		if (thistime > ap->l)
			thistime = ap->l;
//print("reading %d\n", length);
		n = readn(s->fd, ap->p, thistime);
		if (n != thistime)
			goto dead;
		length -= thistime;
	}
	return total;
}

int
nbsswrite(NbSession *s, void *buf, long maxlen)
{
	NbScatterGather a[2];
	a[0].l = maxlen;
	a[0].p = buf;
	a[1].p = nil;
	return nbssgatherwrite(s, a);
}

long
nbssread(NbSession *s, void *buf, long maxlen)
{
	NbScatterGather a[2];
	a[0].l = maxlen;
	a[0].p = buf;
	a[1].p = nil;
	return nbssscatterread(s, a);
}
