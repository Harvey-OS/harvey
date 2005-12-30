#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static struct {
	int thread;
	QLock;
	int fd;
} udp = { -1 };

typedef struct Listen Listen;

struct Listen {
	NbName to;
	int (*deliver)(void *magic, NbDgram *s);
	void *magic;
	Listen *next;
};

static struct {
	Lock;
	ushort id;
} id;

struct {
	QLock;
	NbnsTransaction *head;
} transactionlist;

static void
udplistener(void *)
{	
	for (;;) {
		uchar msg[Udphdrsize + 576];
		int len = read(udp.fd, msg, sizeof(msg));
		if (len < 0)
			break;
		if (len >= nbudphdrsize) {
			NbnsMessage *s;
//			Udphdr *uh;
			uchar *p;

//			uh = (Udphdr*)msg;
			p = msg + nbudphdrsize;
			len -= nbudphdrsize;
			s = nbnsconvM2S(p, len);
			if (s) {
//print("%I:%d -> %I:%d\n", uh->raddr, nhgets(uh->rport), uh->laddr, nhgets(uh->lport));
//nbnsdumpmessage(s);
				if (s->response) {
					NbnsTransaction *t;
					qlock(&transactionlist);
					for (t = transactionlist.head; t; t = t->next)
						if (t->id == s->id)
							break;
					if (t)
						sendp(t->c, s);
					else
						nbnsmessagefree(&s);
					qunlock(&transactionlist);
				}
				else
					nbnsmessagefree(&s);
			}
		}
	}
}

static char *
startlistener(void)
{
	qlock(&udp);
	if (udp.thread < 0) {
		char *e;
		e = nbudpannounce(NbnsPort, &udp.fd);
		if (e) {
			qunlock(&udp);
			return e;
		}
		udp.thread = proccreate(udplistener, nil, 16384);
	}
	qunlock(&udp);
	return nil;
}

ushort
nbnsnextid(void)
{
	ushort rv;
	lock(&id);
	rv = id.id++;
	unlock(&id);
	return rv;
}
	
NbnsTransaction *
nbnstransactionnew(NbnsMessage *s, uchar *ipaddr)
{
	NbnsTransaction *t;
	uchar msg[Udphdrsize + 576];
	Udphdr *u;
	int len;

	startlistener();
	len = nbnsconvS2M(s, msg + nbudphdrsize, sizeof(msg) - nbudphdrsize);
	if (len == 0)
		return 0;
	t = mallocz(sizeof(*t), 1);
	if (t == nil)
		return nil;
	t->id = s->id;	
	t->c = chancreate(sizeof(NbnsMessage *), 3);
	if (t->c == nil) {
		free(t);
		return nil;
	}
	qlock(&transactionlist);
	t->next = transactionlist.head;
	transactionlist.head = t;
	qunlock(&transactionlist);
	u = (Udphdr *)msg;
	ipmove(u->laddr, nbglobals.myipaddr);
	hnputs(u->lport, NbnsPort);
	if (s->broadcast || ipaddr == nil)
		ipmove(u->raddr, nbglobals.bcastaddr);
	else
		ipmove(u->raddr, ipaddr);
	hnputs(u->rport, NbnsPort);
	write(udp.fd, msg, len + nbudphdrsize);
	return t;
}

void
nbnstransactionfree(NbnsTransaction **tp)
{
	NbnsTransaction **tp2;
	NbnsMessage *s;
	NbnsTransaction *t;

	t = *tp;
	if (t) {
		qlock(&transactionlist);
		while ((s = nbrecvp(t->c)) != nil)
			nbnsmessagefree(&s);
		for (tp2 = &transactionlist.head; *tp2 && *tp2 != t; tp2 = &(*tp2)->next)
			;
		if (*tp2) {
			*tp2 = t->next;
			free(t);
		}
		qunlock(&transactionlist);
		*tp = nil;
	}
}
