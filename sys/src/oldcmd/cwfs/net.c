/* network i/o */

#include "all.h"
#include "io.h"
#include <fcall.h>		/* 9p2000 */
#include <thread.h>

enum {
	Maxfdata	= 8192,
	Nqueue		= 200,		/* queue size (tunable) */

	Netclosed	= 0,		/* Connection state */
	Netopen,
};

/*
 * the kernel file server read packets directly from
 * its ethernet(s) and did all the protocol processing.
 * if the incoming packets were 9p (over il/ip), they
 * were queued for the server processes to operate upon.
 *
 * in user mode, we have one process per incoming connection
 * instead, and those processes get just the data, minus
 * tcp and ip headers, so they just see a stream of 9p messages,
 * which they then queue for the server processes.
 *
 * there used to be more queueing (in the kernel), with separate
 * processes for ethernet input, il input, 9p processing, il output
 * and ethernet output, and queues connecting them.  we now let
 * the kernel's network queues, protocol stacks and processes do
 * much of this work.
 *
 * partly as a result of this, we can now process 9p messages
 * transported via tcp, exploit multiple x86 processors, and
 * were able to shed 70% of the file server's source, by line count.
 *
 * the upshot is that Ether (now Network) is no longer a perfect fit for
 * the way network i/o is done now.  the notion of `connection'
 * is being introduced to complement it.
 */

typedef struct Network Network;
typedef struct Netconn Netconn;
typedef struct Conn9p Conn9p;

/* a network, not necessarily an ethernet */
struct Network {
	int	ctlrno;
	char	iname[NAMELEN];
	char	oname[NAMELEN];

	char	*dialstr;
	char	anndir[40];
	char	lisdir[40];
	int	annfd;			/* fd from announce */
};

/* an open tcp (or other transport) connection */
struct Netconn {
	Queue*	reply;		/* network output */
	char*	raddr;		/* remote caller's addr */
	Chan*	chan;		/* list of tcp channels */

	int	alloc;		/* flag: allocated */

	int	state;
	Conn9p*	conn9p;		/* not reference-counted */

	Lock;
};

/*
 * incoming 9P network connection from a given machine.
 * typically will multiplex 9P sessions for multiple users.
 */
struct Conn9p {
	QLock;
	Ref;
	int	fd;
	char*	dir;
	Netconn*netconn;		/* cross-connection */
	char*	raddr;
};

static Network netif[Maxnets];
static struct {
	Lock;
	Chan*	chan;
} netchans;
static Queue *netoq;		/* only one network output queue is needed */

char *annstrs[Maxnets] = {
	"tcp!*!9fs",
};

/* never returns nil */
static Chan*
getchan(Conn9p *conn9p)
{
	Netconn *netconn;
	Chan *cp, *xcp;

	lock(&netchans);

	/* look for conn9p's Chan */
	xcp = nil;
	for(cp = netchans.chan; cp; cp = netconn->chan) {
		netconn = cp->pdata;
		if(!netconn->alloc)
			xcp = cp;		/* remember free Chan */
		else if(netconn->raddr != nil &&
		    strcmp(conn9p->raddr, netconn->raddr) == 0) {
			unlock(&netchans);
			return cp;		/* found conn9p's Chan */
		}
	}

	/* conn9p's Chan not found; if no free Chan, allocate & fill in one */
	cp = xcp;
	if(cp == nil) {
		cp = fs_chaninit(Devnet, 1, sizeof(Netconn));
		netconn = cp->pdata;
		netconn->chan = netchans.chan;
		netconn->state = Netopen;	/* a guess */
		/* cross-connect netconn and conn9p */
		netconn->conn9p = conn9p;	/* not reference-counted */
		conn9p->netconn = netconn;
		netchans.chan = cp;
	}

	/* fill in Chan's netconn */
	netconn = cp->pdata;
	netconn->raddr = strdup(conn9p->raddr);

	/* fill in Chan */
	cp->send = serveq;
	if (cp->reply == nil)
		cp->reply = netoq;
	netconn->reply = netoq;
	cp->protocol = nil;
	cp->msize = 0;
	cp->whotime = 0;
	strncpy(cp->whochan, conn9p->raddr, sizeof cp->whochan);
//	cp->whoprint = tcpwhoprint;
	netconn->alloc = 1;

	unlock(&netchans);
	return cp;
}

static char *
fd2name(int fd)
{
	char data[128];

	if (fd2path(fd, data, sizeof data) < 0)
		return strdup("/GOK");
	return strdup(data);
}

static void
hangupdfd(int dfd)
{
	int ctlfd;
	char *end, *data;

	data = fd2name(dfd);
	close(dfd);

	end = strstr(data, "/data");
	if (end != nil)
		strcpy(end, "/ctl");
	ctlfd = open(data, OWRITE);
	if (ctlfd >= 0) {
		hangup(ctlfd);
		close(ctlfd);
	}
	free(data);
}

void
closechan(int n)
{
	Chan *cp;

	for(cp = chans; cp; cp = cp->next)
		if(cp->whotime != 0 && cp->chan == n)
			fileinit(cp);
}

void
nethangup(Chan *cp, char *msg, int dolock)
{
	Netconn *netconn;

	netconn = cp->pdata;
	netconn->state = Netclosed;

	if(msg != nil)
		print("hangup! %s %s\n", msg, netconn->raddr);

	fileinit(cp);
	cp->whotime = 0;
	strcpy(cp->whoname, "<none>");

	if(dolock)
		lock(&netchans);
	netconn->alloc = 0;
	free(netconn->raddr);
	netconn->raddr = nil;
	if(dolock)
		unlock(&netchans);
}

void
chanhangup(Chan *cp, char *msg, int dolock)
{
	Netconn *netconn = cp->pdata;
	Conn9p *conn9p = netconn->conn9p;

	if (conn9p->fd > 0)
		hangupdfd(conn9p->fd);	/* drop it */
	nethangup(cp, msg, dolock);
}

/*
 * returns length of next 9p message (including the length) and
 * leaves it in the first few bytes of abuf.
 */
static long
size9pmsg(int fd, void *abuf, uint n)
{
	int m;
	uchar *buf = abuf;

	if (n < BIT32SZ)
		return -1;	/* caller screwed up */

	/* read count */
	m = readn(fd, buf, BIT32SZ);
	if(m != BIT32SZ){
		if(m < 0)
			return -1;
		return 0;
	}
	return GBIT32(buf);
}

static int
readalloc9pmsg(int fd, Msgbuf **mbp)
{
	int m, len;
	uchar lenbuf[BIT32SZ];
	Msgbuf *mb;

	*mbp = nil;
	len = size9pmsg(fd, lenbuf, BIT32SZ);
	if (len <= 0)
		return len;
	if(len <= BIT32SZ || len > IOHDRSZ+Maxfdata){
		werrstr("bad length in 9P2000 message header");
		return -1;
	}
	if ((mb = mballoc(len, nil, Mbeth1)) == nil)
		panic("readalloc9pmsg: mballoc failed");
	*mbp = mb;
	memmove(mb->data, lenbuf, BIT32SZ);
	len -= BIT32SZ;
	m = readn(fd, mb->data+BIT32SZ, len);
	if(m < len)
		return 0;
	return BIT32SZ+m;
}

static void
connection(void *v)
{
	int n;
	char buf[64];
	Chan *chan9p;
	Conn9p *conn9p = v;
	Msgbuf *mb;
	NetConnInfo *nci;

	incref(conn9p);			/* count connections */
	nci = getnetconninfo(conn9p->dir, conn9p->fd);
	if (nci == nil)
		panic("connection: getnetconninfo(%s, %d) failed",
			conn9p->dir, conn9p->fd);
	conn9p->raddr = nci->raddr;

	chan9p = getchan(conn9p);
	print("new connection on %s pid %d from %s\n",
		conn9p->dir, getpid(), conn9p->raddr);

	/*
	 * reading from a pipe or a network device
	 * will give an error after a few eof reads.
	 * however, we cannot tell the difference
	 * between a zero-length read and an interrupt
	 * on the processes writing to us,
	 * so we wait for the error.
	 */
	while (conn9p->fd > 0 && (n = readalloc9pmsg(conn9p->fd, &mb)) >= 0) {
		if(n == 0)
			continue;
		mb->param = (uintptr)conn9p;	/* has fd for replies */
		mb->chan = chan9p;

		assert(mb->magic == Mbmagic);
		incref(conn9p);			/* & count packets in flight */
		fs_send(serveq, mb);		/* to 9P server processes */
		/* mb will be freed by receiving process */
	}

	rerrstr(buf, sizeof buf);

	qlock(conn9p);
	print("connection hung up from %s\n", conn9p->dir);
	if (conn9p->fd > 0)		/* not poisoned yet? */
		hangupdfd(conn9p->fd);	/* poison the fd */

	nethangup(chan9p, "remote hung up", 1);
	closechan(chan9p->chan);

	conn9p->fd = -1;		/* poison conn9p */
	if (decref(conn9p) == 0) {	/* last conn.? turn the lights off */
		free(conn9p->dir);
		qunlock(conn9p);
		free(conn9p);
	} else
		qunlock(conn9p);

	freenetconninfo(nci);

	if(buf[0] == '\0' || strstr(buf, "hungup") != nil)
		exits("");
	sysfatal("mount read, pid %d", getpid());
}

static void
neti(void *v)
{
	int lisfd, accfd;
	Network *net;
	Conn9p *conn9p;

	net = v;
	print("net%di\n", net->ctlrno);
	for(;;) {
		lisfd = listen(net->anndir, net->lisdir);
		if (lisfd < 0) {
			print("listen %s failed: %r\n", net->anndir);
			continue;
		}

		/* got new call on lisfd */
		accfd = accept(lisfd, net->lisdir);
		if (accfd < 0) {
			print("accept %d (from %s) failed: %r\n",
				lisfd, net->lisdir);
			continue;
		}

		/* accepted that call */
		conn9p = malloc(sizeof *conn9p);
		conn9p->dir = strdup(net->lisdir);
		conn9p->fd = accfd;
		newproc(connection, conn9p, smprint("9P read %s", conn9p->dir));
		close(lisfd);
	}
}

/* only need one of these for all network connections, thus all interfaces */
static void
neto(void *)
{
	int len, datafd;
	Msgbuf *mb;
	Conn9p *conn9p;

	print("neto\n");
	for(;;) {
		/* receive 9P answer from 9P server processes */
		while((mb = fs_recv(netoq, 0)) == nil)
			continue;

		if(mb->data == nil) {
			print("neto: pkt nil cat=%d free=%d\n",
				mb->category, mb->flags&FREE);
			if(!(mb->flags & FREE))
				mbfree(mb);
			continue;
		}

		/* send answer back over the network connection in the reply */
		len = mb->count;
		conn9p = (Conn9p *)mb->param;
		assert(conn9p);

		qlock(conn9p);
		datafd = conn9p->fd;
		assert(len >= 0);
		/* datafd < 0 probably indicates poisoning by the read side */
		if (datafd < 0 || write(datafd, mb->data, len) != len) {
			print( "network write error (%r);");
			print(" closing connection for %s\n", conn9p->dir);
			nethangup(getchan(conn9p), "network write error", 1);
			if (datafd > 0)
				hangupdfd(datafd);	/* drop it */
			conn9p->fd = -1;		/* poison conn9p */
		}
		mbfree(mb);
		if (decref(conn9p) == 0)
			panic("neto: zero ref count");
		qunlock(conn9p);
	}
}

void
netstart(void)
{
	int netorun = 0;
	Network *net;

	if(netoq == nil)
		netoq = newqueue(Nqueue, "network reply");
	for(net = &netif[0]; net < &netif[Maxnets]; net++){
		if(net->dialstr == nil)
			continue;
		sprint(net->oname, "neto");
		if (netorun++ == 0)
			newproc(neto, nil, net->oname);
		sprint(net->iname, "net%di", net->ctlrno);
		newproc(neti, net, net->iname);
	}
}

void
netinit(void)
{
	Network *net;

	for (net = netif; net < netif + Maxnets; net++) {
		net->dialstr = annstrs[net - netif];
		if (net->dialstr == nil)
			continue;
		net->annfd = announce(net->dialstr, net->anndir);
		/* /bin/service/tcp564 may already have grabbed the port */
		if (net->annfd < 0)
			sysfatal("can't announce %s: %r", net->dialstr);
		print("netinit: announced on %s\n", net->dialstr);
	}
}
