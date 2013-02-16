/*
 *  /net/ssh
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <mp.h>
#include <auth.h>
#include <authsrv.h>
#include <libsec.h>
#include <ip.h>
#include "netssh.h"

extern int nokeyverify;

void stclunk(Fid *);
void stend(Srv *);
void stflush(Req *);
void stopen(Req *);
void stread(Req *);
void stwrite(Req *);

Srv netsshsrv = {
	.open = stopen,
	.read = stread,
	.write = stwrite,
	.flush = stflush,
	.destroyfid = stclunk,
	.end = stend,
};

Cipher *cryptos[] = {
	&cipheraes128,
	&cipheraes192,
	&cipheraes256,
//	&cipherblowfish,
	&cipher3des,
	&cipherrc4,
};

Kex *kexes[] = {
	&dh1sha1,
	&dh14sha1,
};

PKA *pkas[3];

char *macnames[] = {
	"hmac-sha1",
};

char *st_names[] = {
[Empty]		"Empty",
[Allocated]	"Allocated",
[Initting]	"Initting",
[Listening]	"Listening",
[Opening]	"Opening",
[Negotiating]	"Negotiating",
[Authing]	"Authing",
[Established]	"Established",
[Eof]		"Eof",
[Closing]	"Closing",
[Closed]	"Closed",
};

int debug;
int kflag;
char *mntpt = "/net";
char uid[32];
Conn *connections[MAXCONN];
File *rootfile, *clonefile, *ctlfile, *keysfile;
Ioproc *io9p;
MBox keymbox;
QLock availlck;
Rendez availrend;

SSHChan *alloc_chan(Conn *);
Conn *alloc_conn(void);
int auth_req(Packet *, Conn *);
int client_auth(Conn *, Ioproc *);
int dohandshake(Conn *, char *);
char *factlookup(int, int, char *[]);
void filedup(Req *, File *);
void readdata(void *);
void reader(void *);
void readreqrem(void *);
void send_kexinit(Conn *);
void server(char *, char *);
void shutdown(Conn *);
void stlisconn(void *);
void stlischan(void *);
int validatekex(Conn *, Packet *);
int validatekexc(Packet *);
int validatekexs(Packet *);
void writectlproc(void *);
void writedataproc(void *);
void writereqremproc(void *);

static int deferredinit(Conn *c);

static void
sshlogint(Conn *c, char *file, char *p)
{
	char *role, *id;

	if (c == nil)
		role = "";
	else if (c->role == Server)
		role = "server ";
	else
		role = "client ";
	if (c == nil)
		id = strdup("");
	else if (c->user || c->remote)
		id = smprint("user %s@%s id %d ", c->user, c->remote, c->id);
	else
		id = smprint("id %d ", c->id);

	syslog(0, file, "%s: %s%s%s", argv0, role, id, p);
	free(id);
}

void
sshlog(Conn *c, char *fmt, ...)
{
	va_list args;
	char *p;

	/* do this first in case fmt contains "%r" */
	va_start(args, fmt);
	p = vsmprint(fmt, args);
	va_end(args);

	sshlogint(c, "ssh", p);
	sshlogint(c, "sshdebug", p);	/* log in both places */
	free(p);
}

void
sshdebug(Conn *c, char *fmt, ...)
{
	va_list args;
	char *p;

	if (!debug)
		return;

	/* do this first in case fmt contains "%r" */
	va_start(args, fmt);
	p = vsmprint(fmt, args);
	va_end(args);

	sshlogint(c, "sshdebug", p);
	free(p);
}

void
usage(void)
{
	fprint(2, "usage: %s [-dkv] [-m mntpt] [-s srvpt]\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *p, *srvpt = nil;

	quotefmtinstall();
	threadsetname("main");
	nokeyverify = 1;	/* temporary until verification is fixed */
	ARGBEGIN {
	case '9':
		chatty9p = 1;
		break;
	case 'd':
		debug++;
		break;
	case 'k':
		kflag = 1;
		break;
	case 'm':
		mntpt = EARGF(usage());
		break;
	case 's':
		srvpt = EARGF(usage());
		break;
	case 'v':
		nokeyverify = 1;
		break;
	case 'V':
		nokeyverify = 0;
		break;
	default:
		usage();
		break;
	} ARGEND;

	p = getenv("nosshkeyverify");
	if (p && p[0] != '\0')
		nokeyverify = 1;
	free(p);

	if (readfile("/dev/user", uid, sizeof uid) <= 0)
		strcpy(uid, "none");

	keymbox.mchan = chancreate(4, 0);
	availrend.l = &availlck;
	dh_init(pkas);

	/* become a daemon */
	if (rfork(RFNOTEG) < 0)
		fprint(2, "%s: rfork(NOTEG) failed: %r\n", argv0);
	server(mntpt, srvpt);
	threadexits(nil);
}

int
readio(Ioproc *io, int fd, void *buf, int n)
{
	if (io)
		return ioread(io, fd, buf, n);
	else
		return read(fd, buf, n);
}

int
writeio(Ioproc *io, int fd, void *buf, int n)
{
	if (io)
		return iowrite(io, fd, buf, n);
	else
		return write(fd, buf, n);
}

int
read9pmsg(int fd, void *abuf, uint n)
{
	int m, len;
	uchar *buf;

	if (io9p == nil)
		io9p = ioproc();

	buf = abuf;

	/* read count */
	m = ioreadn(io9p, fd, buf, BIT32SZ);
	if(m != BIT32SZ){
		if(m < 0)
			return -1;
		return 0;
	}

	len = GBIT32(buf);
	if(len <= BIT32SZ || len > n){
		werrstr("bad length in 9P2000 message header");
		return -1;
	}
	len -= BIT32SZ;
	m = ioreadn(io9p, fd, buf+BIT32SZ, len);
	if(m < len)
		return 0;
	return BIT32SZ+m;
}

void
stend(Srv *)
{
	closeioproc(io9p);
	threadkillgrp(threadgetgrp());
}

void
server(char *mntpt, char *srvpt)
{
	Dir d;
	char *p;
	int fd;

	netsshsrv.tree = alloctree(uid, uid, 0777, nil);
	rootfile = createfile(netsshsrv.tree->root, "ssh", uid, 0555|DMDIR,
		(void*)Qroot);
	clonefile = createfile(rootfile, "clone", uid, 0666, (void*)Qclone);
	ctlfile = createfile(rootfile, "ctl", uid, 0666, (void*)Qctl);
	keysfile = createfile(rootfile, "keys", uid, 0600, (void *)Qreqrem);

	/*
	 * needs to be MBEFORE in case there are previous, now defunct,
	 * netssh processes mounted in mntpt.
	 */
	threadpostmountsrv(&netsshsrv, srvpt, mntpt, MBEFORE);

	p = esmprint("%s/cs", mntpt);
	fd = open(p, OWRITE);
	free(p);
	if (fd >= 0) {
		fprint(fd, "add ssh");
		close(fd);
	}
	if (srvpt) {
		nulldir(&d);
		d.mode = 0666;
		p = esmprint("/srv/%s", srvpt);
		dirwstat(p, &d);
		free(p);
	}
	sshdebug(nil, "server started for %s", getuser());
}

static void
respexit(Conn *c, Req *r, void *freeme, char *msg)
{
	if (msg)
		sshdebug(c, "%s", msg);
	r->aux = 0;
	respond(r, msg);
	free(freeme);
	threadexits(nil);	/* maybe use msg here */
}

void
stopen(Req *r)
{
	int lev, xconn, fd;
	uvlong qidpath;
	char *p;
	char buf[32];
	Conn *c;
	SSHChan *sc;

	qidpath = (uvlong)r->fid->file->aux;
	lev = qidpath >> Levshift;
	switch ((ulong)(qidpath & Qtypemask)) {
	default:
		respond(r, nil);
		break;
	case Qlisten:
		r->aux = (void *)threadcreate((lev == Connection?
			stlisconn: stlischan), r, Defstk);
		break;
	case Qclone:
		switch (lev) {
		case Top:
			/* should use dial(2) instead of diddling /net/tcp */
			p = esmprint("%s/tcp/clone", mntpt);
			fd = open(p, ORDWR);
			if (fd < 0) {
				sshdebug(nil, "stopen: open %s failed: %r", p);
				free(p);
				responderror(r);
				return;
			}
			free(p);

			c = alloc_conn();
			if (c == nil) {
				close(fd);
				respond(r, "no more connections");
				return;
			}
			c->ctlfd = fd;
			c->poisoned = 0;
			filedup(r, c->ctlfile);
			sshlog(c, "new connection on fd %d", fd);
			break;
		case Connection:
			xconn = (qidpath >> Connshift) & Connmask;
			c = connections[xconn];
			if (c == nil) {
				respond(r, "bad connection");
				return;
			}
			sc = alloc_chan(c);
			if (sc == nil) {
				respond(r, "no more channels");
				return;
			}
			filedup(r, sc->ctl);
			break;
		default:
			snprint(buf, sizeof buf, "bad level %d", lev);
			readstr(r, buf);
			break;
		}
		respond(r, nil);
		break;
	}
}

static void
listerrexit(Req *r, Ioproc *io, Conn *cl)
{
	r->aux = 0;
	responderror(r);
	closeioproc(io);
	shutdown(cl);
	threadexits(nil);
}

void
stlisconn(void *a)
{
	int xconn, fd, n;
	uvlong qidpath;
	char *msg;
	char buf[Numbsz], path[NETPATHLEN];
	Conn *c, *cl;
	Ioproc *io;
	Req *r;

	threadsetname("stlisconn");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	xconn = (qidpath >> Connshift) & Connmask;

	cl = connections[xconn];
	if (cl == nil) {
		sshlog(cl, "bad connection");
		respond(r, "bad connection");
		threadexits("bad connection");
	}
	if (cl->poisoned) {
		sshdebug(cl, "stlisconn conn %d poisoned", xconn);
		r->aux = 0;
		respond(r, "top level listen conn poisoned");
		threadexits("top level listen conn poisoned");
	}
	if (cl->ctlfd < 0) {
		sshdebug(cl, "stlisconn conn %d ctlfd < 0; poisoned", xconn);
		r->aux = 0;
		respond(r, "top level listen with closed fd");
		shutdown(cl);
		cl->poisoned = 1;	/* no more use until ctlfd is set */
		threadexits("top level listen with closed fd");
	}

	io = ioproc();

	/* read xconn's tcp conn's ctl file */
	seek(cl->ctlfd, 0, 0);
	n = ioread(io, cl->ctlfd, buf, sizeof buf - 1);
	if (n == 0) {
		sshlog(cl, "stlisconn read eof on fd %d", cl->ctlfd);
		listerrexit(r, io, cl);
	} else if (n < 0) {
		sshlog(cl, "stlisconn read failed on fd %d: %r", cl->ctlfd);
		listerrexit(r, io, cl);
	}
	buf[n] = '\0';

	cl->state = Listening;
	/* should use dial(2) instead of diddling /net/tcp */
	snprint(path, sizeof path, "%s/tcp/%s/listen", mntpt, buf);
	for(;;) {
		fd = ioopen(io, path, ORDWR);
		if (fd < 0) 
			listerrexit(r, io, cl);
		c = alloc_conn();
		if (c)
			break;
		n = ioread(io, fd, buf, sizeof buf - 1);
		if (n <= 0)
			listerrexit(r, io, cl);
		buf[n] = '\0';
		msg = smprint("reject %s no available connections", buf);
		iowrite(io, fd, msg, strlen(msg));
		free(msg);
		close(fd);			/* surely ioclose? */
	}
	c->ctlfd = fd;
	if (c->ctlfd < 0) {
		sshlog(cl, "stlisconn c->ctlfd < 0 for conn %d", xconn);
		threadexitsall("stlisconn c->ctlfd < 0");
	}
	c->poisoned = 0;
	c->stifle = 1;			/* defer server; was for coexistence */
	filedup(r, c->ctlfile);
	sshdebug(c, "responding to listen open");
	r->aux = 0;
	respond(r, nil);
	closeioproc(io);
	threadexits(nil);
}

void
stlischan(void *a)
{
	Req *r;
	Packet *p2;
	Ioproc *io;
	Conn *c;
	SSHChan *sc;
	int i, n, xconn;
	uvlong qidpath;

	threadsetname("stlischan");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		respond(r, "bad channel");
		sshlog(c, "bad channel");
		threadexits(nil);
	}
	if (c->state == Closed || c->state == Closing)
		respexit(c, r, nil, "channel listen on closed connection");
	sc = c->chans[qidpath & Chanmask];

	qlock(&c->l);
	sc->lreq = r;
	for (i = 0; i < c->nchan; ++i)
		if (c->chans[i] && c->chans[i]->state == Opening &&
		    c->chans[i]->ann && strcmp(c->chans[i]->ann, sc->ann) == 0)
			break;
	if (i >= c->nchan) {
		sc->state = Listening;
		rsleep(&sc->r);
		i = sc->waker;
		if (i < 0) {
			qunlock(&c->l);
			r->aux = 0;
			responderror(r);
			threadexits(nil);
		}
	} else
		rwakeup(&c->chans[i]->r);
	qunlock(&c->l);

	if (c->state == Closed || c->state == Closing || c->state == Eof)
		respexit(c, r, nil, "channel listen on closed connection");
	c->chans[i]->state = Established;

	p2 = new_packet(c);
	c->chans[i]->rwindow = Maxpayload;
	add_byte(p2, SSH_MSG_CHANNEL_OPEN_CONFIRMATION);
	hnputl(p2->payload + 1, c->chans[i]->otherid);
	hnputl(p2->payload + 5, c->chans[i]->id);
	hnputl(p2->payload + 9, Maxpayload);
	hnputl(p2->payload + 13, Maxrpcbuf);
	p2->rlength = 18;
	n = finish_packet(p2);
	filedup(r, c->chans[i]->ctl);

	io = ioproc();
	n = iowrite(io, c->datafd, p2->nlength, n);
	closeioproc(io);

	free(p2);

	sshdebug(c, "responding to chan listen open");
	r->aux = 0;
	if (n < 0)
		responderror(r);
	else
		respond(r, nil);
	threadexits(nil);
}

void
getdata(Conn *c, SSHChan *sc, Req *r)
{
	Packet *p;
	Plist *d;
	int n;

	n = r->ifcall.count;
	if (sc->dataq->rem < n)
		n = sc->dataq->rem;
	if (n > Maxrpcbuf)
		n = Maxrpcbuf;
	r->ifcall.offset = 0;

	readbuf(r, sc->dataq->st, n);
	sc->dataq->st += n;
	sc->dataq->rem -= n;
	sc->inrqueue -= n;
	if (sc->dataq->rem <= 0) {
		d = sc->dataq;
		sc->dataq = sc->dataq->next;
		if (d->pack->tlength > sc->rwindow)
			sc->rwindow = 0;
		else
			sc->rwindow -= d->pack->tlength;
		free(d->pack);
		free(d);
	}
	if (sc->rwindow < 16*1024) {		/* magic.  half-way, maybe? */
		sc->rwindow += Maxpayload;
		sshdebug(c, "increasing receive window to %lud, inq %lud\n",
			argv0, sc->rwindow, sc->inrqueue);
		p = new_packet(c);
		add_byte(p, SSH_MSG_CHANNEL_WINDOW_ADJUST);
		hnputl(p->payload+1, sc->otherid);
		hnputl(p->payload+5, Maxpayload);
		p->rlength += 8;
		n = finish_packet(p);
		iowrite(c->dio, c->datafd, p->nlength, n);
		free(p);
	}
	r->aux = 0;
	respond(r, nil);
}

void
stread(Req *r)
{
	Conn *c;
	SSHChan *sc;
	int n, lev, cnum, xconn;
	uvlong qidpath;
	char buf[Arbbufsz], path[NETPATHLEN];

	threadsetname("stread");
	qidpath = (uvlong)r->fid->file->aux;
	lev = qidpath >> Levshift;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		if (lev != Top || (qidpath & Qtypemask) != Qreqrem) {
			respond(r, "Invalid connection");
			return;
		}
		cnum = 0;
		sc = nil;
	} else {
		cnum = qidpath & Chanmask;
		sc = c->chans[cnum];
	}
	switch ((ulong)(qidpath & Qtypemask)) {
	case Qctl:
	case Qlisten:
		if (r->ifcall.offset != 0) {
			respond(r, nil);
			break;
		}
		switch (lev) {
		case Top:
			readstr(r, st_names[c->state]);
			break;
		case Connection:
		case Subchannel:
			snprint(buf, sizeof buf, "%d", lev == Connection?
				xconn: cnum);
			readstr(r, buf);
			break;
		default:
			snprint(buf, sizeof buf, "stread error, level %d", lev);
			respond(r, buf);
			return;
		}
		respond(r, nil);
		break;
	case Qclone:
		if (r->ifcall.offset != 0) {
			respond(r, nil);
			break;
		}
		readstr(r, "Congratulations, you've achieved the impossible\n");
		respond(r, nil);
		break;
	case Qdata:
		if (lev == Top) {
			respond(r, nil);
			break;
		}
		if (lev == Connection) {
			if (0 && c->stifle) {	/* was for coexistence */
				c->stifle = 0;
				if (deferredinit(c) < 0) {
					respond(r, "deferredinit failed");
					break;
				}
			}
			if (c->cap)			/* auth capability? */
				readstr(r, c->cap);
			respond(r, nil);
			break;
		}

		r->aux = (void *)threadcreate(readdata, r, Defstk);
		break;
	case Qlocal:
		if (lev == Connection)
			if (c->ctlfd < 0)
				readstr(r, "::!0\n");
			else {
				n = pread(c->ctlfd, buf, 10, 0); // magic 10
				buf[n >= 0? n: 0] = '\0';
				snprint(path, sizeof path, "%s/tcp/%s/local",
					mntpt, buf);
				readfile(path, buf, sizeof buf);
				readstr(r, buf);
			}
		respond(r, nil);
		break;
	case Qreqrem:
		r->aux = (void *)threadcreate(readreqrem, r, Defstk);
		break;
	case Qstatus:
		switch (lev) {
		case Top:
			readstr(r, "Impossible");
			break;
		case Connection:
			readstr(r, (uint)c->state > Closed?
				"Unknown": st_names[c->state]);
			break;
		case Subchannel:
			readstr(r, (uint)sc->state > Closed?
				"Unknown": st_names[sc->state]);
			break;
		}
		respond(r, nil);
		break;
	case Qtcp:
		/* connection number of underlying tcp connection */
		if (lev == Connection)
			if (c->ctlfd < 0)
				readstr(r, "-1\n");
			else {
				n = pread(c->ctlfd, buf, 10, 0); /* magic 10 */
				buf[n >= 0? n: 0] = '\0';
				readstr(r, buf);
			}
		respond(r, nil);
		break;
	default:
		respond(r, nil);
		break;
	}
}

void
readreqrem(void *a)
{
	Ioproc *io;
	Req *r;
	Conn *c;
	SSHChan *sc;
	int fd, n, lev, cnum, xconn;
	uvlong qidpath;
	char buf[Arbbufsz], path[NETPATHLEN];

	threadsetname("readreqrem");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	lev = qidpath >> Levshift;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		if (lev != Top) {
			respond(r, "Invalid connection");
			return;
		}
		sc = nil;
	} else {
		cnum = qidpath & Chanmask;
		sc = c->chans[cnum];
	}
	switch (lev) {
	case Top:
		if (r->ifcall.offset == 0 && keymbox.state != Empty) {
			r->aux = 0;
			respond(r, "Key file collision");	/* WTF? */
			break;
		}
		if (r->ifcall.offset != 0) {
			readstr(r, keymbox.msg);
			r->aux = 0;
			respond(r, nil);
			if (r->ifcall.offset + r->ifcall.count >=
			    strlen(keymbox.msg))
				keymbox.state = Empty;
			else
				keymbox.state = Allocated;
			break;
		}
		keymbox.state = Allocated;
		for(;;) {
			if (keymbox.msg == nil)
				if (recv(keymbox.mchan, nil) < 0) {
					r->aux = 0;
					responderror(r);
					keymbox.state = Empty;
					threadexits(nil);
				}
			if (keymbox.state == Empty)
				break;
			else if (keymbox.state == Allocated) {
				if (keymbox.msg) {
					readstr(r, keymbox.msg);
					if (r->ifcall.offset + r->ifcall.count
					    >= strlen(keymbox.msg)) {
						free(keymbox.msg);
						keymbox.msg = nil;
						keymbox.state = Empty;
					}
				}
				break;
			}
		}
		r->aux = 0;
		respond(r, nil);
		break;
	case Connection:
		if (c->ctlfd >= 0) {
			io = ioproc();
			seek(c->ctlfd, 0, 0);
			n = ioread(io, c->ctlfd, buf, 10); /* magic 10 */
			if (n < 0) {
				r->aux = 0;
				responderror(r);
				closeioproc(io);
				break;
			}
			buf[n] = '\0';
			snprint(path, NETPATHLEN, "%s/tcp/%s/remote", mntpt, buf);
			if ((fd = ioopen(io, path, OREAD)) < 0 ||
			    (n = ioread(io, fd, buf, Arbbufsz - 1)) < 0) {
				r->aux = 0;
				responderror(r);
				if (fd >= 0)
					ioclose(io, fd);
				closeioproc(io);
				break;
			}
			ioclose(io, fd);
			closeioproc(io);
			buf[n] = '\0';
			readstr(r, buf);
		} else
			readstr(r, "::!0\n");
		r->aux = 0;
		respond(r, nil);
		break;
	case Subchannel:
		if ((sc->state == Closed || sc->state == Closing ||
		    sc->state == Eof) && sc->reqq == nil && sc->dataq == nil) {
			sshdebug(c, "sending EOF1 to channel request listener");
			r->aux = 0;
			respond(r, nil);
			break;
		}
		while (sc->reqq == nil) {
			if (recv(sc->reqchan, nil) < 0) {
				r->aux = 0;
				responderror(r);
				threadexits(nil);
			}
			if ((sc->state == Closed || sc->state == Closing ||
			    sc->state == Eof) && sc->reqq == nil &&
			    sc->dataq == nil) {
				sshdebug(c, "sending EOF2 to channel request "
					"listener");
				respexit(c, r, nil, nil);
			}
		}
		n = r->ifcall.count;
		if (sc->reqq->rem < n)
			n = sc->reqq->rem;
		if (n > Maxrpcbuf)
			n = Maxrpcbuf;
		r->ifcall.offset = 0;
		readbuf(r, sc->reqq->st, n);
		sc->reqq->st += n;
		sc->reqq->rem -= n;
		if (sc->reqq->rem <= 0) {
			Plist *d = sc->reqq;
			sc->reqq = sc->reqq->next;
			free(d->pack);
			free(d);
		}
		r->aux = 0;
		respond(r, nil);
		break;
	}
	threadexits(nil);
}

void
readdata(void *a)
{
	Req *r;
	Conn *c;
	SSHChan *sc;
	int cnum, xconn;
	uvlong qidpath;

	threadsetname("readdata");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		respond(r, "bad connection");
		sshlog(c, "bad connection");
		threadexits(nil);
	}
	cnum = qidpath & Chanmask;
	sc = c->chans[cnum];
	if (sc->dataq == nil && (sc->state == Closed || sc->state == Closing ||
	    sc->state == Eof)) {
		sshdebug(c, "sending EOF1 to channel listener");
		r->aux = 0;
		respond(r, nil);
		threadexits(nil);
	}
	if (sc->dataq != nil) {
		getdata(c, sc, r);
		threadexits(nil);
	}
	while (sc->dataq == nil) {
		if (recv(sc->inchan, nil) < 0) {
			sshdebug(c, "got interrupt/error in readdata %r");
			r->aux = 0;
			responderror(r);
			threadexits(nil);
		}
		if (sc->dataq == nil && (sc->state == Closed ||
		    sc->state == Closing || sc->state == Eof)) {
			sshdebug(c, "sending EOF2 to channel listener");
			r->aux = 0;
			respond(r, nil);
			threadexits(nil);
		}
	}
	getdata(c, sc, r);
	threadexits(nil);
}

void
stwrite(Req *r)
{
	Conn *c;
	SSHChan *ch;
	int lev, xconn;
	uvlong qidpath;

	threadsetname("stwrite");
	qidpath = (uvlong)r->fid->file->aux;
	lev = qidpath >> Levshift;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		respond(r, "invalid connection");
		return;
	}
	ch = c->chans[qidpath & Chanmask];
	switch ((ulong)(qidpath & Qtypemask)) {
	case Qclone:
	case Qctl:
		r->aux = (void *)threadcreate(writectlproc, r, Defstk);
		break;
	case Qdata:
		r->ofcall.count = r->ifcall.count;
		if (lev == Top || lev == Connection ||
		    c->state == Closed || c->state == Closing ||
		    ch->state == Closed || ch->state == Closing) {
			respond(r, nil);
			break;
		}
		if (0 && c->stifle) {		/* was for coexistence */
			c->stifle = 0;
			if (deferredinit(c) < 0) {
				respond(r, "deferredinit failed");
				break;
			}
		}
		r->aux = (void *)threadcreate(writedataproc, r, Defstk);
		break;
	case Qreqrem:
		r->aux = (void *)threadcreate(writereqremproc, r, Defstk);
		break;
	default:
		respond(r, nil);
		break;
	}
}

static int
dialbyhand(Conn *c, int ntok, char *toks[])
{
	/*
	 * this uses /net/tcp to connect directly.
	 * should use dial(2) instead of doing it by hand.
	 */
	sshdebug(c, "tcp connect %s %s", toks[1], ntok > 3? toks[2]: "");
	return fprint(c->ctlfd, "connect %s %s", toks[1], ntok > 3? toks[2]: "");
}

static void
userauth(Conn *c, Req *r, char *buf, int ntok, char *toks[])
{
	int n;
	char *attrs[5];
	Packet *p;

	if (ntok < 3 || ntok > 4)
		respexit(c, r, buf, "bad connect command");
	if (!c->service)
		c->service = estrdup9p(toks[0]);
	if (c->user)
		free(c->user);
	c->user = estrdup9p(toks[2]);
	sshdebug(c, "userauth for user %s", c->user);

	if (ntok == 4 && strcmp(toks[1], "k") == 0) {
		if (c->authkey) {
			free(c->authkey);
			c->authkey = nil;
		}
		if (c->password)
			free(c->password);
		c->password = estrdup9p(toks[3]);
		sshdebug(c, "userauth got password");
	} else {
		if (c->password) {
			free(c->password);
			c->password = nil;
		}
		memset(attrs, 0, sizeof attrs);
		attrs[0] = "proto=rsa";
		attrs[1] = "!dk?";
		attrs[2] = smprint("user=%s", c->user);
		attrs[3] = smprint("sys=%s", c->remote);
		if (c->authkey)
			free(c->authkey);
		sshdebug(c, "userauth trying rsa");
		if (ntok == 3)
			c->authkey = factlookup(4, 2, attrs);
		else {
			attrs[4] = toks[3];
			c->authkey = factlookup(5, 2, attrs);
		}
		free(attrs[2]);
		free(attrs[3]);
	}

	if (!c->password && !c->authkey)
		respexit(c, r, buf, "no auth info");
	else if (c->state != Authing) {
		p = new_packet(c);
		add_byte(p, SSH_MSG_SERVICE_REQUEST);
		add_string(p, c->service);
		n = finish_packet(p);
		sshdebug(c, "sending msg svc req for %s", c->service);
		if (writeio(c->dio, c->datafd, p->nlength, n) != n) {
			sshdebug(c, "authing write failed: %r");
			free(p);
			r->aux = 0;
			responderror(r);
			free(buf);
			threadexits(nil);
		}
		free(p);
	} else
		if (client_auth(c, c->dio) < 0)
			respexit(c, r, buf, "ssh-userauth client auth failed");
	qlock(&c->l);
	if (c->state != Established) {
		sshdebug(c, "sleeping for auth");
		rsleep(&c->r);
	}
	qunlock(&c->l);
	if (c->state != Established)
		respexit(c, r, buf, "ssh-userath auth failed (not Established)");
}

void
writectlproc(void *a)
{
	Req *r;
	Packet *p;
	Conn *c;
	SSHChan *ch;
	char *tcpconn2, *buf, *toks[4];
	int n, ntok, lev, xconn;
	uvlong qidpath;
	char path[NETPATHLEN], tcpconn[Numbsz];

	threadsetname("writectlproc");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	lev = qidpath >> Levshift;
	xconn = (qidpath >> Connshift) & Connmask;

	c = connections[xconn];
	if (c == nil) {
		respond(r, "bad connection");
		sshlog(c, "bad connection");
		threadexits(nil);
	}
	ch = c->chans[qidpath & Chanmask];

	if (r->ifcall.count <= Numbsz)
		buf = emalloc9p(Numbsz + 1);
	else
		buf = emalloc9p(r->ifcall.count + 1);
	memmove(buf, r->ifcall.data, r->ifcall.count);
	buf[r->ifcall.count] = '\0';

	sshdebug(c, "level %d writectl: %s", lev, buf);
	ntok = tokenize(buf, toks, nelem(toks));
	switch (lev) {
	case Connection:
		if (strcmp(toks[0], "id") == 0) {	/* was for sshswitch */
			if (ntok < 2)
				respexit(c, r, buf, "bad id request");
			strncpy(c->idstring, toks[1], sizeof c->idstring);
			sshdebug(c, "id %s", toks[1]);
			break;
		}
		if (strcmp(toks[0], "connect") == 0) {
			if (ntok < 2)
				respexit(c, r, buf, "bad connect request");
			/*
			 * should use dial(2) instead of doing it by hand.
			 */
			memset(tcpconn, '\0', sizeof(tcpconn));
			pread(c->ctlfd, tcpconn, sizeof tcpconn, 0);
			dialbyhand(c, ntok, toks);

			c->role = Client;
			/* Override the PKA list; we can take any in */
			pkas[0] = &rsa_pka;
			pkas[1] = &dss_pka;
			pkas[2] = nil;
			tcpconn2 = estrdup9p(tcpconn);

			/* swap id strings, negotiate crypto */
			if (dohandshake(c, tcpconn2) < 0) {
				sshlog(c, "connect handshake failed: "
					"tcp conn %s", tcpconn2);
				free(tcpconn2);
				respexit(c, r, buf, "connect handshake failed");
			}
			free(tcpconn2);
			keymbox.state = Empty;
			nbsendul(keymbox.mchan, 1);
			break;
		}

		if (c->state == Closed || c->state == Closing)
			respexit(c, r, buf, "connection closed");
		if (strcmp(toks[0], "ssh-userauth") == 0)
			userauth(c, r, buf, ntok, toks);
		else if (strcmp(toks[0], "ssh-connection") == 0) {
			/* your ad here */
		} else if (strcmp(toks[0], "hangup") == 0) {
			if (c->rpid >= 0)
				threadint(c->rpid);
			shutdown(c);
		} else if (strcmp(toks[0], "announce") == 0) {
			sshdebug(c, "got %s argument for announce", toks[1]);
			write(c->ctlfd, r->ifcall.data, r->ifcall.count);
		} else if (strcmp(toks[0], "accept") == 0) {
			/* should use dial(2) instead of diddling /net/tcp */
			memset(tcpconn, '\0', sizeof(tcpconn));
			pread(c->ctlfd, tcpconn, sizeof tcpconn, 0);
			fprint(c->ctlfd, "accept %s", tcpconn);

			c->role = Server;
			tcpconn2 = estrdup9p(tcpconn);
			/* swap id strings, negotiate crypto */
			if (dohandshake(c, tcpconn2) < 0) {
				sshlog(c, "accept handshake failed: "
					"tcp conn %s", tcpconn2);
				free(tcpconn2);
				shutdown(c);
				respexit(c, r, buf, "accept handshake failed");
			}
			free(tcpconn2);
		} else if (strcmp(toks[0], "reject") == 0) {
			memset(tcpconn, '\0', sizeof(tcpconn));
			pread(c->ctlfd, tcpconn, sizeof tcpconn, 0);

			snprint(path, NETPATHLEN, "%s/tcp/%s/data", mntpt, tcpconn);
			c->datafd = open(path, ORDWR);

			p = new_packet(c);
			add_byte(p, SSH_MSG_DISCONNECT);
			add_byte(p, SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT);
			add_string(p, toks[2]);
			add_string(p, "EN");
			n = finish_packet(p);
			if (c->dio && c->datafd >= 0)
				iowrite(c->dio, c->datafd, p->nlength, n);
			free(p);
			if (c->ctlfd >= 0)
				fprint(c->ctlfd, "reject %s %s", buf, toks[2]);
			if (c->rpid >= 0)
				threadint(c->rpid);
			shutdown(c);
		}
		break;
	case Subchannel:
		if (c->state == Closed || c->state == Closing)
			respexit(c, r, buf, "channel closed");
		if (strcmp(toks[0], "connect") == 0) {
			p = new_packet(c);
			add_byte(p, SSH_MSG_CHANNEL_OPEN);
			sshdebug(c, "chan writectl: connect %s",
				ntok > 1? toks[1]: "session");
			add_string(p, ntok > 1? toks[1]: "session");
			add_uint32(p, ch->id);
			add_uint32(p, Maxpayload);
			add_uint32(p, Maxrpcbuf);
			/* more stuff if it's an x11 session */
			n = finish_packet(p);
			iowrite(c->dio, c->datafd, p->nlength, n);
			free(p);
			qlock(&c->l);
			if (ch->otherid == -1)
				rsleep(&ch->r);
			qunlock(&c->l);
		} else if (strcmp(toks[0], "global") == 0) {
			/* your ad here */
		} else if (strcmp(toks[0], "hangup") == 0) {
			if (ch->state != Closed && ch->state != Closing) {
				ch->state = Closing;
				if (ch->otherid != -1) {
					p = new_packet(c);
					add_byte(p, SSH_MSG_CHANNEL_CLOSE);
					add_uint32(p, ch->otherid);
					n = finish_packet(p);
					iowrite(c->dio, c->datafd, p->nlength, n);
					free(p);
				}
				qlock(&c->l);
				rwakeup(&ch->r);
				qunlock(&c->l);
				nbsendul(ch->inchan, 1);
				nbsendul(ch->reqchan, 1);
			}
			for (n = 0; n < MAXCONN && (c->chans[n] == nil ||
			    c->chans[n]->state == Empty ||
			    c->chans[n]->state == Closing ||
			    c->chans[n]->state == Closed); ++n)
				;
			if (n >= MAXCONN) {
				if (c->rpid >= 0)
					threadint(c->rpid);
				shutdown(c);
			}
		} else if (strcmp(toks[0], "announce") == 0) {
			sshdebug(c, "got argument `%s' for chan announce",
				toks[1]);
			free(ch->ann);
			ch->ann = estrdup9p(toks[1]);
		}
		break;
	}
	r->ofcall.count = r->ifcall.count;
	r->aux = 0;
	respond(r, nil);
	free(buf);
	threadexits(nil);
}

void
writereqremproc(void *a)
{
	Req *r;
	Packet *p;
	Conn *c;
	SSHChan *ch;
	char *cmd, *q, *buf, *toks[4];
	int n, ntok, lev, xconn;
	uvlong qidpath;

	threadsetname("writereqremproc");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	lev = qidpath >> Levshift;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		respond(r, "Invalid connection");
		threadexits(nil);
	}
	ch = c->chans[qidpath & Chanmask];
	if (r->ifcall.count <= 10)
		buf = emalloc9p(10 + 1);
	else
		buf = emalloc9p(r->ifcall.count + 1);
	memmove(buf, r->ifcall.data, r->ifcall.count);
	buf[r->ifcall.count] = '\0';
	sshdebug(c, "writereqrem: %s", buf);
	ntok = tokenize(buf, toks, nelem(toks));

	if (lev == Top) {
		free(keymbox.msg);
		keymbox.msg = buf;
		nbsendul(keymbox.mchan, 1);
		r->ofcall.count = r->ifcall.count;
		respexit(c, r, nil, nil);
	}

	r->ofcall.count = r->ifcall.count;
	if (c->state == Closed  || c->state == Closing ||
	    ch->state == Closed || ch->state == Closing)
		respexit(c, r, buf, nil);

	p = new_packet(c);
	if (strcmp(toks[0], "success") == 0) {
		add_byte(p, SSH_MSG_CHANNEL_SUCCESS);
		add_uint32(p, ch->otherid);
	} else if (strcmp(toks[0], "failure") == 0) {
		add_byte(p, SSH_MSG_CHANNEL_FAILURE);
		add_uint32(p, ch->otherid);
	} else if (strcmp(toks[0], "close") == 0) {
		ch->state = Closing;
		add_byte(p, SSH_MSG_CHANNEL_CLOSE);
		add_uint32(p, ch->otherid);
	} else if (strcmp(toks[0], "shell") == 0) {
		ch->state = Established;
		/*
		 * Some servers *cough*OpenSSH*cough* don't seem to be able
		 * to intelligently handle a shell with no pty.
		 */
		add_byte(p, SSH_MSG_CHANNEL_REQUEST);
		add_uint32(p, ch->otherid);
		add_string(p, "pty-req");
		add_byte(p, 0);
		if (ntok == 1)
			add_string(p, "dumb");
		else
			add_string(p, toks[1]);
		add_uint32(p, 0);
		add_uint32(p, 0);
		add_uint32(p, 0);
		add_uint32(p, 0);
		add_string(p, "");
		n = finish_packet(p);
		iowrite(c->dio, c->datafd, p->nlength, n);
		init_packet(p);
		p->c = c;
		add_byte(p, SSH_MSG_CHANNEL_REQUEST);
		add_uint32(p, ch->otherid);
		add_string(p, "shell");
		add_byte(p, 0);
		sshdebug(c, "sending shell request: rlength=%lud twindow=%lud",
			p->rlength, ch->twindow);
	} else if (strcmp(toks[0], "exec") == 0) {
		ch->state = Established;
		add_byte(p, SSH_MSG_CHANNEL_REQUEST);
		add_uint32(p, ch->otherid);
		add_string(p, "exec");
		add_byte(p, 0);

		cmd = emalloc9p(Bigbufsz);
		q = seprint(cmd, cmd+Bigbufsz, "%s", toks[1]);
		for (n = 2; n < ntok; ++n) {
			q = seprint(q, cmd+Bigbufsz, " %q", toks[n]);
			if (q == nil)
				break;
		}
		add_string(p, cmd);
		free(cmd);
	} else
		respexit(c, r, buf, "bad request command");
	n = finish_packet(p);
	iowrite(c->dio, c->datafd, p->nlength, n);
	free(p);
	respexit(c, r, buf, nil);
}

void
writedataproc(void *a)
{
	Req *r;
	Packet *p;
	Conn *c;
	SSHChan *ch;
	int n, xconn;
	uvlong qidpath;

	threadsetname("writedataproc");
	r = a;
	qidpath = (uvlong)r->fid->file->aux;
	xconn = (qidpath >> Connshift) & Connmask;
	c = connections[xconn];
	if (c == nil) {
		respond(r, "Invalid connection");
		threadexits(nil);
	}
	ch = c->chans[qidpath & Chanmask];

	p = new_packet(c);
	add_byte(p, SSH_MSG_CHANNEL_DATA);
	hnputl(p->payload+1, ch->otherid);
	p->rlength += 4;
	add_block(p, r->ifcall.data, r->ifcall.count);
	n = finish_packet(p);

	if (ch->sent + p->rlength > ch->twindow) {
		qlock(&ch->xmtlock);
		while (ch->sent + p->rlength > ch->twindow)
			rsleep(&ch->xmtrendez);
		qunlock(&ch->xmtlock);
	}
	iowrite(c->dio, c->datafd, p->nlength, n);
	respexit(c, r, p, nil);
}

/*
 * Although this is named stclunk, it's attached to the destroyfid
 * member of the Srv struct.  It turns out there's no member
 * called clunk.  But if there are no other references, a 9P Tclunk
 * will end up calling destroyfid.
 */
void
stclunk(Fid *f)
{
	Packet *p;
	Conn *c;
	SSHChan *sc;
	int n, lev, cnum, chnum;
	uvlong qidpath;

	threadsetname("stclunk");
	if (f == nil || f->file == nil)
		return;
	qidpath = (uvlong)f->file->aux;
	lev = qidpath >> Levshift;
	cnum = (qidpath >> Connshift) & Connmask;
	chnum = qidpath & Chanmask;
	c = connections[cnum];
	sshdebug(c, "got clunk on file: %#llux %d %d %d: %s",
		qidpath, lev, cnum, chnum, f->file->name);
	/* qidpath test implies conn 0, chan 0 */
	if (lev == Top && qidpath == Qreqrem) {
		if (keymbox.state != Empty) {
			keymbox.state = Empty;
			// nbsendul(keymbox.mchan, 1);
		}
		keymbox.msg = nil;
		return;
	}

	if (c == nil)
		return;
	if (lev == Connection && (qidpath & Qtypemask) == Qctl &&
	    (c->state == Opening || c->state == Negotiating ||
	     c->state == Authing)) {
		for (n = 0; n < MAXCONN && (!c->chans[n] ||
		    c->chans[n]->state == Empty ||
		    c->chans[n]->state == Closed ||
		    c->chans[n]->state == Closing); ++n)
			;
		if (n >= MAXCONN) {
			if (c->rpid >= 0)
				threadint(c->rpid);
			shutdown(c);
		}
		return;
	}

	sc = c->chans[chnum];
	if (lev != Subchannel)
		return;
	if ((qidpath & Qtypemask) == Qlisten && sc->state == Listening) {
		qlock(&c->l);
		if (sc->state != Closed) {
			sc->state = Closed;
			chanclose(sc->inchan);
			chanclose(sc->reqchan);
		}
		qunlock(&c->l);
	} else if ((qidpath & Qtypemask) == Qdata && sc->state != Empty &&
	    sc->state != Closed && sc->state != Closing) {
		if (f->file != sc->data && f->file != sc->request) {
			sshlog(c, "great evil is upon us; destroying a fid "
				"we didn't create");
			return;
		}

		p = new_packet(c);
		add_byte(p, SSH_MSG_CHANNEL_CLOSE);
		hnputl(p->payload+1, sc->otherid);
		p->rlength += 4;
		n = finish_packet(p);
		sc->state = Closing;
		iowrite(c->dio, c->datafd, p->nlength, n);
		free(p);

		qlock(&c->l);
		rwakeup(&sc->r);
		qunlock(&c->l);
		nbsendul(sc->inchan, 1);
		nbsendul(sc->reqchan, 1);
	}
	for (n = 0; n < MAXCONN && (!c->chans[n] ||
	    c->chans[n]->state == Empty || c->chans[n]->state == Closed ||
	    c->chans[n]->state == Closing); ++n)
		;
	if (n >= MAXCONN) {
		if (c->rpid >= 0)
			threadint(c->rpid);
		shutdown(c);
	}
}

void
stflush(Req *r)
{
	Req *or;
	uvlong qidpath;

	threadsetname("stflush");
	or = r->oldreq;
	qidpath = (uvlong)or->fid->file->aux;
	sshdebug(nil, "got flush on file %#llux %lld %lld %lld: %s %#p",
		argv0, qidpath, qidpath >> Levshift,
		(qidpath >> Connshift) & Connmask, qidpath & Chanmask,
		or->fid->file->name, or->aux);
	if (!or->aux)
		respond(or, "interrupted");
	else if (or->ifcall.type == Topen && (qidpath & Qtypemask) == Qlisten ||
	    or->ifcall.type == Tread && (qidpath & Qtypemask) == Qdata &&
	    (qidpath >> Levshift) == Subchannel ||
	    or->ifcall.type == Tread && (qidpath & Qtypemask) == Qreqrem)
		threadint((uintptr)or->aux);
	else {
		threadkill((uintptr)or->aux);
		or->aux = 0;
		respond(or, "interrupted");
	}
	respond(r, nil);
}

void
filedup(Req *r, File *src)
{
	r->ofcall.qid = src->qid;
	closefile(r->fid->file);
	r->fid->file = src;
	incref(src);
}

Conn *
alloc_conn(void)
{
	int slevconn, i, s, firstnil;
	char buf[Numbsz];
	Conn *c;
	static QLock aclock;

	qlock(&aclock);
	firstnil = -1;
	for (i = 0; i < MAXCONN; ++i) {
		if (connections[i] == nil) {
			if (firstnil == -1)
				firstnil = i;
			continue;
		}
		s = connections[i]->state;
		if (s == Empty || s == Closed)
			break;
	}
	if (i >= MAXCONN) {
		if (firstnil == -1) {		/* all slots in use? */
			qunlock(&aclock);
			return nil;
		}
		/* no reusable slots, allocate a new Conn */
		connections[firstnil] = emalloc9p(sizeof(Conn));
		memset(connections[firstnil], 0, sizeof(Conn));
		i = firstnil;
	}

	c = connections[i];
	memset(&c->r, '\0', sizeof(Rendez));
	c->r.l = &c->l;
	c->dio = ioproc();
	c->rio = nil;
	c->state = Allocated;
	c->role = Server;
	c->id = i;
	c->stifle = c->poisoned = 0;
	c->user = c->service = nil;
	c->inseq = c->nchan = c->outseq = 0;
	c->cscrypt = c->csmac = c->ctlfd = c->datafd = c->decrypt =
		c->encrypt = c->inmac = c->ncscrypt = c->ncsmac =
		c->nsccrypt = c->nscmac = c->outmac = c->rpid = c->sccrypt =
		c->scmac = c->tcpconn = -1;
	if (c->e) {
		mpfree(c->e);
		c->e = nil;
	}
	if (c->x) {
		mpfree(c->x);
		c->x = nil;
	}

	snprint(buf, sizeof buf, "%d", i);
	if (c->dir == nil) {
		slevconn = Connection << Levshift | i << Connshift;
		c->dir = createfile(rootfile, buf, uid, 0555|DMDIR,
			(void *)(slevconn | Qroot));
		c->clonefile = createfile(c->dir, "clone", uid, 0666,
			(void *)(slevconn | Qclone));
		c->ctlfile = createfile(c->dir, "ctl", uid, 0666,
			(void *)(slevconn | Qctl));
		c->datafile = createfile(c->dir, "data", uid, 0666,
			(void *)(slevconn | Qdata));
		c->listenfile = createfile(c->dir, "listen", uid, 0666,
			(void *)(slevconn | Qlisten));
		c->localfile = createfile(c->dir, "local", uid, 0444,
			(void *)(slevconn | Qlocal));
		c->remotefile = createfile(c->dir, "remote", uid, 0444,
			(void *)(slevconn | Qreqrem));
		c->statusfile = createfile(c->dir, "status", uid, 0444,
			(void *)(slevconn | Qstatus));
		c->tcpfile = createfile(c->dir, "tcp", uid, 0444,
			(void *)(slevconn | Qtcp));
	}
//	c->skexinit = c->rkexinit = nil;
	c->got_sessid = 0;
	c->otherid = nil;
	c->inik = c->outik = nil;
	c->s2ccs = c->c2scs = c->enccs = c->deccs = nil;
	qunlock(&aclock);
	return c;
}

SSHChan *
alloc_chan(Conn *c)
{
	int cnum, slcn;
	char buf[Numbsz];
	Plist *p, *next;
	SSHChan *sc;

	if (c->nchan >= MAXCONN)
		return nil;
	qlock(&c->l);
	cnum = c->nchan;
	if (c->chans[cnum] == nil) {
		c->chans[cnum] = emalloc9p(sizeof(SSHChan));
		memset(c->chans[cnum], 0, sizeof(SSHChan));
	}
	sc = c->chans[cnum];
	snprint(buf, sizeof buf, "%d", cnum);
	memset(&sc->r, '\0', sizeof(Rendez));
	sc->r.l = &c->l;
	sc->id = cnum;
	sc->state = Empty;
	sc->conn = c->id;
	sc->otherid = sc->waker = -1;
	sc->sent = sc->twindow = sc->rwindow = sc->inrqueue = 0;
	sc->ann = nil;
	sc->lreq = nil;

	if (sc->dir == nil) {
		slcn = Subchannel << Levshift | c->id << Connshift | cnum;
		sc->dir = createfile(c->dir, buf, uid, 0555|DMDIR,
			(void *)(slcn | Qroot));
		sc->ctl = createfile(sc->dir, "ctl", uid, 0666,
			(void *)(slcn | Qctl));
		sc->data = createfile(sc->dir, "data", uid, 0666,
			(void *)(slcn | Qdata));
		sc->listen = createfile(sc->dir, "listen", uid, 0666,
			(void *)(slcn | Qlisten));
		sc->request = createfile(sc->dir, "request", uid, 0666,
			(void *)(slcn | Qreqrem));
		sc->status = createfile(sc->dir, "status", uid, 0444,
			(void *)(slcn | Qstatus));
		sc->tcp = createfile(sc->dir, "tcp", uid, 0444,
			(void *)(slcn | Qtcp));
	}
	c->nchan++;

	for (; sc->reqq != nil; sc->reqq = next) {
		p = sc->reqq;
		next = p->next;
		free(p->pack);
		free(p);
	}
	sc->dataq = sc->datatl = sc->reqtl = nil;

	if (sc->inchan)
		chanfree(sc->inchan);
	sc->inchan = chancreate(4, 0);

	if (sc->reqchan)
		chanfree(sc->reqchan);
	sc->reqchan = chancreate(4, 0);

	memset(&sc->xmtrendez, '\0', sizeof(Rendez));
	sc->xmtrendez.l = &sc->xmtlock;
	qunlock(&c->l);
	return sc;
}

static int
readlineio(Conn *, Ioproc *io, int fd, char *buf, int size)
{
	int n;
	char *p;

	for (p = buf; p < buf + size - 1; p++) {
		n = ioread(io, fd, p, 1);
		if (n != 1 || *p == '\n') {
			*p = '\0';
			break;
		}
	}
	return p - buf;
}

static char *
readremote(Conn *c, Ioproc *io, char *tcpconn)
{
	int n, remfd;
	char *p, *remote;
	char path[Arbbufsz], buf[NETPATHLEN];

	remote = nil;
	snprint(path, sizeof path, "%s/tcp/%s/remote", mntpt, tcpconn);
	remfd = ioopen(io, path, OREAD);
	if (remfd < 0) {
		sshlog(c, "readremote: can't open %s: %r", path);
		return nil;
	}
	n = ioread(io, remfd, buf, sizeof buf - 1);
	if (n > 0) {
		buf[n] = 0;
		p = strchr(buf, '!');
		if (p)
			*p = 0;
		remote = estrdup9p(buf);
	}
	ioclose(io, remfd);
	return remote;
}

static void
sendmyid(Conn *c, Ioproc *io)
{
	char path[Arbbufsz];

	snprint(path, sizeof path, "%s\r\n", MYID);
	iowrite(io, c->datafd, path, strlen(path));
}

/* save and tidy up the remote id */
static void
stashremid(Conn *c, char *remid)
{
	char *nl;

	if (c->otherid)
		free(c->otherid);
	c->otherid = estrdup9p(remid);

	nl = strchr(c->otherid, '\n');
	if (nl)
		*nl = '\0';
	nl = strchr(c->otherid, '\r');
	if (nl)
		*nl = '\0';
}

static void
hangupconn(Conn *c)
{
	hangup(c->ctlfd);
	close(c->ctlfd);
	close(c->datafd);
	c->ctlfd = c->datafd = -1;
}

#ifdef COEXIST
static int
exchids(Conn *c, Ioproc *io, char *remid, int remsz)
{
	int n;

	/*
	 * exchange versions.  server writes id, then reads;
	 * client reads id then writes (in theory).
	 */
	if (c->role == Server) {
		sendmyid(c, io);

		n = readlineio(c, io, c->datafd, remid, remsz);
		if (n < 5)		/* can't be a valid SSH id string */
			return -1;
		sshdebug(c, "dohandshake: server, got `%s', sent `%s'", remid,
			MYID);
	} else {
		/* client: read server's id */
		n = readlineio(c, io, c->datafd, remid, remsz);
		if (n < 5)		/* can't be a valid SSH id string */
			return -1;

		sendmyid(c, io);
		sshdebug(c, "dohandshake: client, got `%s' sent `%s'", remid, MYID);
		if (remid[0] == '\0') {
			sshlog(c, "dohandshake: client, empty remote id string;"
				" out of sync");
			return -1;
		}
	}
	sshdebug(c, "remote id string `%s'", remid);
	return 0;
}

/*
 * negotiate the protocols.
 * We don't do the full negotiation here, because we also have
 * to handle a re-negotiation request from the other end.
 * So we just kick it off and let the receiver process take it from there.
 */
static int
negotiate(Conn *c)
{
	send_kexinit(c);

	qlock(&c->l);
	if ((c->role == Client && c->state != Negotiating) ||
	    (c->role == Server && c->state != Established)) {
		sshdebug(c, "awaiting establishment");
		rsleep(&c->r);
	}
	qunlock(&c->l);

	if (c->role == Server && c->state != Established ||
	    c->role == Client && c->state != Negotiating) {
		sshdebug(c, "failed to establish");
		return -1;
	}
	sshdebug(c, "established; crypto now on");
	return 0;
}

/* this was deferred when trying to make coexistence with v1 work */
static int
deferredinit(Conn *c)
{
	char remid[Arbbufsz];
	Ioproc *io;

	io = ioproc();
	/*
	 * don't bother checking the remote's id string.
	 * as a client, we can cope with v1 if we don't verify the host key.
	 */
	if (exchids(c, io, remid, sizeof remid) < 0 ||
	    0 && c->role == Client && strncmp(remid, "SSH-2", 5) != 0 &&
	    strncmp(remid, "SSH-1.99", 8) != 0) {
		/* not a protocol version we know; give up */
		closeioproc(io);
		hangupconn(c);
		return -1;
	}
	closeioproc(io);
	stashremid(c, remid);

	c->state = Initting;

	/* start the reader thread */
	if (c->rpid < 0)
		c->rpid = threadcreate(reader, c, Defstk);

	return negotiate(c);
}

int
dohandshake(Conn *c, char *tcpconn)
{
	int tcpdfd;
	char *remote;
	char path[Arbbufsz];
	Ioproc *io;

	io = ioproc();

	/* read tcp conn's remote address into c->remote */
	remote = readremote(c, io, tcpconn);
	if (remote) {
		free(c->remote);
		c->remote = remote;
	}

	/* open tcp conn's data file */
	c->tcpconn = atoi(tcpconn);
	snprint(path, sizeof path, "%s/tcp/%s/data", mntpt, tcpconn);
	tcpdfd = ioopen(io, path, ORDWR);
	closeioproc(io);
	if (tcpdfd < 0) {
		sshlog(c, "dohandshake: can't open %s: %r", path);
		return -1;
	}
	c->datafd = tcpdfd;		/* underlying tcp data descriptor */

	return deferredinit(c);
}
#endif					/* COEXIST */

int
dohandshake(Conn *c, char *tcpconn)
{
	int fd, n;
	char *p, *othid;
	char path[Arbbufsz], buf[NETPATHLEN];
	Ioproc *io;

	io = ioproc();
	snprint(path, sizeof path, "%s/tcp/%s/remote", mntpt, tcpconn);
	fd = ioopen(io, path, OREAD);
	n = ioread(io, fd, buf, sizeof buf - 1);
	if (n > 0) {
		buf[n] = 0;
		p = strchr(buf, '!');
		if (p)
			*p = 0;
		free(c->remote);
		c->remote = estrdup9p(buf);
	}
	ioclose(io, fd);

	snprint(path, sizeof path, "%s/tcp/%s/data", mntpt, tcpconn);
	fd = ioopen(io, path, ORDWR);
	if (fd < 0) {
		closeioproc(io);
		return -1;
	}
	c->datafd = fd;

	/* exchange versions--we're only doing SSH2, unfortunately */

	snprint(path, sizeof path, "%s\r\n", MYID);
	if (c->idstring && c->idstring[0])
		strncpy(path, c->idstring, sizeof path);
	else {
		iowrite(io, fd, path, strlen(path));
		p = path;
		n = 0;
		do {
			if (ioread(io, fd, p, 1) < 0) {
				fprint(2, "%s: short read in ID exchange: %r\n",
					argv0);
				break;
			}
			++n;
		} while (*p++ != '\n');
		if (n < 5) {		/* can't be a valid SSH id string */
			close(fd);
			goto err;
		}
		*p = 0;
	}
	sshdebug(c, "id string `%s'", path);
	if (c->idstring[0] == '\0' &&
	    strncmp(path, "SSH-2", 5) != 0 &&
	    strncmp(path, "SSH-1.99", 8) != 0) {
		/* not a protocol version we know; give up */
		ioclose(io, fd);
		goto err;
	}
	closeioproc(io);

	if (c->otherid)
		free(c->otherid);
	c->otherid = othid = estrdup9p(path);
	for (n = strlen(othid) - 1; othid[n] == '\r' || othid[n] == '\n'; --n)
		othid[n] = '\0';
	c->state = Initting;

	/* start the reader thread */
	if (c->rpid < 0)
		c->rpid = threadcreate(reader, c, Defstk);

	/*
	 * negotiate the protocols
	 * We don't do the full negotiation here, because we also have
	 * to handle a re-negotiation request from the other end.  So
	 * we just kick it off and let the receiver process take it from there.
	 */

	send_kexinit(c);

	qlock(&c->l);
	if ((c->role == Client && c->state != Negotiating) ||
	    (c->role == Server && c->state != Established))
		rsleep(&c->r);
	qunlock(&c->l);
	if (c->role == Server && c->state != Established ||
	    c->role == Client && c->state != Negotiating)
		return -1;
	return 0;
err:
	/* should use hangup in dial(2) instead of diddling /net/tcp */
	snprint(path, sizeof path, "%s/tcp/%s/ctl", mntpt, tcpconn);
	fd = ioopen(io, path, OWRITE);
	iowrite(io, fd, "hangup", 6);
	ioclose(io, fd);
	closeioproc(io);
	return -1;
}

void
send_kexinit(Conn *c)
{
	Packet *ptmp;
	char *buf, *p, *e;
	int i, msglen;

	sshdebug(c, "initializing kexinit packet");
	if (c->skexinit != nil)
		free(c->skexinit);
	c->skexinit = new_packet(c);

	buf = emalloc9p(Bigbufsz);
	buf[0] = (uchar)SSH_MSG_KEXINIT;

	add_packet(c->skexinit, buf, 1);
	for (i = 0; i < 16; ++i)
		buf[i] = fastrand();

	add_packet(c->skexinit, buf, 16);		/* cookie */
	e = buf + Bigbufsz - 1;
	p = seprint(buf, e, "%s", kexes[0]->name);
	for (i = 1; i < nelem(kexes); ++i)
		p = seprint(p, e, ",%s", kexes[i]->name);
	sshdebug(c, "sent KEX algs: %s", buf);

	add_string(c->skexinit, buf);		/* Key exchange */
	if (pkas[0] == nil)
		add_string(c->skexinit, "");
	else{
		p = seprint(buf, e, "%s", pkas[0]->name);
		for (i = 1; i < nelem(pkas) && pkas[i] != nil; ++i)
			p = seprint(p, e, ",%s", pkas[i]->name);
		sshdebug(c, "sent host key algs: %s", buf);
		add_string(c->skexinit, buf);		/* server's key algs */
	}

	p = seprint(buf, e, "%s", cryptos[0]->name);
	for (i = 1; i < nelem(cryptos); ++i)
		p = seprint(p, e, ",%s", cryptos[i]->name);
	sshdebug(c, "sent crypto algs: %s", buf);

	add_string(c->skexinit, buf);		/* c->s crypto */
	add_string(c->skexinit, buf);		/* s->c crypto */
	p = seprint(buf, e, "%s", macnames[0]);
	for (i = 1; i < nelem(macnames); ++i)
		p = seprint(p, e, ",%s", macnames[i]);
	sshdebug(c, "sent MAC algs: %s", buf);

	add_string(c->skexinit, buf);		/* c->s mac */
	add_string(c->skexinit, buf);		/* s->c mac */
	add_string(c->skexinit, "none");	/* c->s compression */
	add_string(c->skexinit, "none");	/* s->c compression */
	add_string(c->skexinit, "");		/* c->s languages */
	add_string(c->skexinit, "");		/* s->c languages */
	memset(buf, 0, 5);
	add_packet(c->skexinit, buf, 5);

	ptmp = new_packet(c);
	memmove(ptmp, c->skexinit, sizeof(Packet));
	msglen = finish_packet(ptmp);

	if (c->dio && c->datafd >= 0)
		iowrite(c->dio, c->datafd, ptmp->nlength, msglen);
	free(ptmp);
	free(buf);
}

static void
establish(Conn *c)
{
	qlock(&c->l);
	c->state = Established;
	rwakeup(&c->r);
	qunlock(&c->l);
}

static int
negotiating(Conn *c, Packet *p, Packet *p2, char *buf, int size)
{
	int i, n;

	USED(size);
	switch (p->payload[0]) {
	case SSH_MSG_DISCONNECT:
		if (debug) {
			get_string(p, p->payload + 5, buf, Arbbufsz, nil);
			sshdebug(c, "got disconnect: %s", buf);
		}
		return -1;
	case SSH_MSG_NEWKEYS:
		/*
		 * If we're just updating, go straight to
		 * established, otherwise wait for auth'n.
		 */
		i = c->encrypt;
		memmove(c->c2siv, c->nc2siv, SHA1dlen*2);
		memmove(c->s2civ, c->ns2civ, SHA1dlen*2);
		memmove(c->c2sek, c->nc2sek, SHA1dlen*2);
		memmove(c->s2cek, c->ns2cek, SHA1dlen*2);
		memmove(c->c2sik, c->nc2sik, SHA1dlen*2);
		memmove(c->s2cik, c->ns2cik, SHA1dlen*2);
		c->cscrypt = c->ncscrypt;
		c->sccrypt = c->nsccrypt;
		c->csmac = c->ncsmac;
		c->scmac = c->nscmac;
		c->c2scs = cryptos[c->cscrypt]->init(c, 0);
		c->s2ccs = cryptos[c->sccrypt]->init(c, 1);
		if (c->role == Server) {
			c->encrypt = c->sccrypt;
			c->decrypt = c->cscrypt;
			c->outmac = c->scmac;
			c->inmac = c->csmac;
			c->enccs = c->s2ccs;
			c->deccs = c->c2scs;
			c->outik = c->s2cik;
			c->inik = c->c2sik;
		} else{
			c->encrypt = c->cscrypt;
			c->decrypt = c->sccrypt;
			c->outmac = c->csmac;
			c->inmac = c->scmac;
			c->enccs = c->c2scs;
			c->deccs = c->s2ccs;
			c->outik = c->c2sik;
			c->inik = c->s2cik;
		}
		sshdebug(c, "using %s for encryption and %s for decryption",
			cryptos[c->encrypt]->name, cryptos[c->decrypt]->name);
		qlock(&c->l);
		if (i != -1)
			c->state = Established;
		if (c->role == Client)
			rwakeup(&c->r);
		qunlock(&c->l);
		break;
	case SSH_MSG_KEXDH_INIT:
		kexes[c->kexalg]->serverkex(c, p);
		break;
	case SSH_MSG_KEXDH_REPLY:
		init_packet(p2);
		p2->c = c;
		if (kexes[c->kexalg]->clientkex2(c, p) < 0) {
			add_byte(p2, SSH_MSG_DISCONNECT);
			add_byte(p2, SSH_DISCONNECT_KEY_EXCHANGE_FAILED);
			add_string(p2, "Key exchange failure");
			add_string(p2, "");
			n = finish_packet(p2);
			iowrite(c->rio, c->datafd, p2->nlength, n);
			shutdown(c);
			free(p);
			free(p2);
			closeioproc(c->rio);
			c->rio = nil;
			c->rpid = -1;

			qlock(&c->l);
			rwakeup(&c->r);
			qunlock(&c->l);

			sshlog(c, "key exchange failure");
			threadexits(nil);
		}
		add_byte(p2, SSH_MSG_NEWKEYS);
		n = finish_packet(p2);
		iowrite(c->rio, c->datafd, p2->nlength, n);
		qlock(&c->l);
		rwakeup(&c->r);
		qunlock(&c->l);
		break;
	case SSH_MSG_SERVICE_REQUEST:
		get_string(p, p->payload + 1, buf, Arbbufsz, nil);
		sshdebug(c, "got service request: %s", buf);
		if (strcmp(buf, "ssh-userauth") == 0 ||
		    strcmp(buf, "ssh-connection") == 0) {
			init_packet(p2);
			p2->c = c;
			sshdebug(c, "connection");
			add_byte(p2, SSH_MSG_SERVICE_ACCEPT);
			add_string(p2, buf);
			n = finish_packet(p2);
			iowrite(c->rio, c->datafd, p2->nlength, n);
			c->state = Authing;
		} else{
			init_packet(p2);
			p2->c = c;
			add_byte(p2, SSH_MSG_DISCONNECT);
			add_byte(p2, SSH_DISCONNECT_SERVICE_NOT_AVAILABLE);
			add_string(p2, "Unknown service type");
			add_string(p2, "");
			n = finish_packet(p2);
			iowrite(c->rio, c->datafd, p2->nlength, n);
			return -1;
		}
		break;
	case SSH_MSG_SERVICE_ACCEPT:
		get_string(p, p->payload + 1, buf, Arbbufsz, nil);
		if (c->service && strcmp(c->service, "ssh-userauth") == 0) {
			free(c->service);
			c->service = estrdup9p("ssh-connection");
		}
		sshdebug(c, "got service accept: %s: responding with %s %s",
			buf, c->user, c->service);
		n = client_auth(c, c->rio);
		c->state = Authing;
		if (n < 0) {
			qlock(&c->l);
			rwakeup(&c->r);
			qunlock(&c->l);
		}
		break;
	}
	return 0;
}

static void
nochans(Conn *c, Packet *p, Packet *p2)
{
	int n;

	init_packet(p2);
	p2->c = c;
	add_byte(p2, SSH_MSG_CHANNEL_OPEN_FAILURE);
	add_block(p2, p->payload + 5, 4);
	hnputl(p2->payload + p2->rlength - 1, 4);
	p2->rlength += 4;
	add_string(p2, "No available channels");
	add_string(p2, "EN");
	n = finish_packet(p2);
	iowrite(c->rio, c->datafd, p2->nlength, n);
}

static int
established(Conn *c, Packet *p, Packet *p2, char *buf, int size)
{
	int i, n, cnum;
	uchar *q;
	Plist *pl;
	SSHChan *ch;

	USED(size);
	if (debug > 1) {
		sshdebug(c, "in Established state, got:");
		dump_packet(p);
	}
	switch (p->payload[0]) {
	case SSH_MSG_DISCONNECT:
		if (debug) {
			get_string(p, p->payload + 5, buf, Arbbufsz, nil);
			sshdebug(c, "got disconnect: %s", buf);
		}
		return -1;
	case SSH_MSG_IGNORE:
	case SSH_MSG_UNIMPLEMENTED:
		break;
	case SSH_MSG_DEBUG:
		if (debug || p->payload[1]) {
			get_string(p, p->payload + 2, buf, Arbbufsz, nil);
			sshdebug(c, "got debug message: %s", buf);
		}
		break;
	case SSH_MSG_KEXINIT:
		send_kexinit(c);
		if (c->rkexinit)
			free(c->rkexinit);
		c->rkexinit = new_packet(c);
		memmove(c->rkexinit, p, sizeof(Packet));
		if (validatekex(c, p) < 0) {
			sshdebug(c, "kex crypto algorithm mismatch (Established)");
			return -1;
		}
		sshdebug(c, "using %s Kex algorithm and %s PKA",
			kexes[c->kexalg]->name, pkas[c->pkalg]->name);
		c->state = Negotiating;
		break;
	case SSH_MSG_GLOBAL_REQUEST:
	case SSH_MSG_REQUEST_SUCCESS:
	case SSH_MSG_REQUEST_FAILURE:
		break;
	case SSH_MSG_CHANNEL_OPEN:
		q = get_string(p, p->payload + 1, buf, Arbbufsz, nil);
		sshdebug(c, "searching for a listener for channel type %s", buf);
		ch = alloc_chan(c);
		if (ch == nil) {
			nochans(c, p, p2);
			break;
		}

		sshdebug(c, "alloced channel %d for listener", ch->id);
		qlock(&c->l);
		ch->otherid = nhgetl(q);
		ch->twindow = nhgetl(q+4);
		sshdebug(c, "got lock in channel open");
		for (i = 0; i < c->nchan; ++i)
			if (c->chans[i] && c->chans[i]->state == Listening &&
			    c->chans[i]->ann &&
			    strcmp(c->chans[i]->ann, buf) == 0)
				break;
		if (i >= c->nchan) {
			sshdebug(c, "no listener: sleeping");
			ch->state = Opening;
			if (ch->ann)
				free(ch->ann);
			ch->ann = estrdup9p(buf);
			sshdebug(c, "waiting for someone to announce %s", ch->ann);
			rsleep(&ch->r);
		} else{
			sshdebug(c, "found listener on channel %d", ch->id);
			c->chans[i]->waker = ch->id;
			rwakeup(&c->chans[i]->r);
		}
		qunlock(&c->l);
		break;
	case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		qlock(&c->l);
		ch->otherid = nhgetl(p->payload+5);
		ch->twindow = nhgetl(p->payload+9);
		rwakeup(&ch->r);
		qunlock(&c->l);
		break;
	case SSH_MSG_CHANNEL_OPEN_FAILURE:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		qlock(&c->l);
		rwakeup(&ch->r);
		qunlock(&c->l);
		return -1;
	case SSH_MSG_CHANNEL_WINDOW_ADJUST:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		ch->twindow += nhgetl(p->payload + 5);
		sshdebug(c, "new twindow for channel: %d: %lud", cnum, ch->twindow);
		qlock(&ch->xmtlock);
		rwakeup(&ch->xmtrendez);
		qunlock(&ch->xmtlock);
		break;
	case SSH_MSG_CHANNEL_DATA:
	case SSH_MSG_CHANNEL_EXTENDED_DATA:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		pl = emalloc9p(sizeof(Plist));
		pl->pack = emalloc9p(sizeof(Packet));
		memmove(pl->pack, p, sizeof(Packet));
		if (p->payload[0] == SSH_MSG_CHANNEL_DATA) {
			pl->rem = nhgetl(p->payload + 5);
			pl->st = pl->pack->payload + 9;
		} else {
			pl->rem = nhgetl(p->payload + 9);
			pl->st = pl->pack->payload + 13;
		}
		pl->next = nil;
		if (ch->dataq == nil)
			ch->dataq = pl;
		else
			ch->datatl->next = pl;
		ch->datatl = pl;
		ch->inrqueue += pl->rem;
		nbsendul(ch->inchan, 1);
		break;
	case SSH_MSG_CHANNEL_EOF:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		if (ch->state != Closed && ch->state != Closing) {
			ch->state = Eof;
			nbsendul(ch->inchan, 1);
			nbsendul(ch->reqchan, 1);
		}
		break;
	case SSH_MSG_CHANNEL_CLOSE:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		if (ch->state != Closed && ch->state != Closing) {
			init_packet(p2);
			p2->c = c;
			add_byte(p2, SSH_MSG_CHANNEL_CLOSE);
			hnputl(p2->payload + 1, ch->otherid);
			p2->rlength += 4;
			n = finish_packet(p2);
			iowrite(c->rio, c->datafd, p2->nlength, n);
		}
		qlock(&c->l);
		if (ch->state != Closed) {
			ch->state = Closed;
			rwakeup(&ch->r);
			nbsendul(ch->inchan, 1);
			nbsendul(ch->reqchan, 1);
			chanclose(ch->inchan);
			chanclose(ch->reqchan);
		}
		qunlock(&c->l);
		for (i = 0; i < MAXCONN && (!c->chans[i] ||
		    c->chans[i]->state == Empty || c->chans[i]->state == Closed);
		    ++i)
			;
		if (i >= MAXCONN)
			return -1;
		break;
	case SSH_MSG_CHANNEL_REQUEST:
		cnum = nhgetl(p->payload + 1);
		ch = c->chans[cnum];
		sshdebug(c, "queueing channel request for channel: %d", cnum);
		q = get_string(p, p->payload+5, buf, Arbbufsz, nil);
		pl = emalloc9p(sizeof(Plist));
		pl->pack = emalloc9p(sizeof(Packet));
		n = snprint((char *)pl->pack->payload,
			Maxpayload, "%s %c", buf, *q? 't': 'f');
		sshdebug(c, "request message begins: %s",
			(char *)pl->pack->payload);
		memmove(pl->pack->payload + n, q + 1, p->rlength - (11 + n-2));
		pl->rem = p->rlength - 11 + 2;
		pl->st = pl->pack->payload;
		pl->next = nil;
		if (ch->reqq == nil)
			ch->reqq = pl;
		else
			ch->reqtl->next = pl;
		ch->reqtl = pl;
		nbsendul(ch->reqchan, 1);
		break;
	case SSH_MSG_CHANNEL_SUCCESS:
	case SSH_MSG_CHANNEL_FAILURE:
	default:
		break;
	}
	return 0;
}

static void
bail(Conn *c, Packet *p, Packet *p2, char *sts)
{
	shutdown(c);
	free(p);
	free(p2);
	if (c->rio) {
		closeioproc(c->rio);
		c->rio = nil;
	}
	c->rpid = -1;
	threadexits(sts);
}

static void
reader0(Conn *c, Packet *p, Packet *p2)
{
	int i, n, nl, np, nm, nb;
	char buf[Arbbufsz];

	nm = 0;
	nb = 4;
	if (c->decrypt != -1)
		nb = cryptos[c->decrypt]->blklen;
	sshdebug(c, "calling read for connection %d, state %d, nb %d, dc %d",
		c->id, c->state, nb, c->decrypt);
	if ((nl = ioreadn(c->rio, c->datafd, p->nlength, nb)) != nb) {
		sshdebug(c, "reader for connection %d exiting, got %d: %r",
			c->id, nl);
		bail(c, p, p2, "reader exiting");
	}
	if (c->decrypt != -1)
		cryptos[c->decrypt]->decrypt(c->deccs, p->nlength, nb);
	p->rlength = nhgetl(p->nlength);
	sshdebug(c, "got message length: %ld", p->rlength);
	if (p->rlength > Maxpktpay) {
		sshdebug(c, "absurd packet length: %ld, unrecoverable decrypt failure",
			p->rlength);
		bail(c, p, p2, "absurd packet length");
	}
	np = ioreadn(c->rio, c->datafd, p->nlength + nb, p->rlength + 4 - nb);
	if (c->inmac != -1)
		nm = ioreadn(c->rio, c->datafd, p->nlength + p->rlength + 4,
			SHA1dlen);		/* SHA1dlen was magic 20 */
	n = nl + np + nm;
	if (debug) {
		sshdebug(c, "got message of %d bytes %d padding", n, p->pad_len);
		if (p->payload[0] > SSH_MSG_CHANNEL_OPEN) {
			i = nhgetl(p->payload+1);
			if (c->chans[i])
				sshdebug(c, " for channel %d win %lud",
					i, c->chans[i]->rwindow);
			else
				sshdebug(c, " for invalid channel %d", i);
		}
		sshdebug(c, " first byte: %d", p->payload[0]);
	}
	/* SHA1dlen was magic 20 */
	if (np != p->rlength + 4 - nb || c->inmac != -1 && nm != SHA1dlen) {
		sshdebug(c, "got EOF/error on connection read: %d %d %r", np, nm);
		bail(c, p, p2, "error or eof");
	}
	p->tlength = n;
	p->rlength = n - 4;
	if (undo_packet(p) < 0) {
		sshdebug(c, "bad packet in connection %d: exiting", c->id);
		bail(c, p, p2, "bad packet");
	}

	if (c->state == Initting) {
		if (p->payload[0] != SSH_MSG_KEXINIT) {
			sshdebug(c, "missing KEX init packet: %d", p->payload[0]);
			bail(c, p, p2, "bad kex");
		}
		if (c->rkexinit)
			free(c->rkexinit);
		c->rkexinit = new_packet(c);
		memmove(c->rkexinit, p, sizeof(Packet));
		if (validatekex(c, p) < 0) {
			sshdebug(c, "kex crypto algorithm mismatch (Initting)");
			bail(c, p, p2, "bad kex");
		}
		sshdebug(c, "using %s Kex algorithm and %s PKA",
			kexes[c->kexalg]->name, pkas[c->pkalg]->name);
		if (c->role == Client)
			kexes[c->kexalg]->clientkex1(c, p);
		c->state = Negotiating;
	} else if (c->state == Negotiating) {
		if (negotiating(c, p, p2, buf, sizeof buf) < 0)
			bail(c, p, p2, "negotiating");
	} else if (c->state == Authing) {
		switch (p->payload[0]) {
		case SSH_MSG_DISCONNECT:
			if (debug) {
				get_string(p, p->payload + 5, buf, Arbbufsz, nil);
				sshdebug(c, "got disconnect: %s", buf);
			}
			bail(c, p, p2, "msg disconnect");
		case SSH_MSG_USERAUTH_REQUEST:
			switch (auth_req(p, c)) {
			case 0:			/* success */
				establish(c);
				break;
			case 1:			/* ok to try again */
			case -1:		/* failure */
				break;
			case -2:		/* can't happen, now at least */
				bail(c, p, p2, "in userauth request");
			}
			break;
		case SSH_MSG_USERAUTH_FAILURE:
			qlock(&c->l);
			rwakeup(&c->r);
			qunlock(&c->l);
			break;
		case SSH_MSG_USERAUTH_SUCCESS:
			establish(c);
			break;
		case SSH_MSG_USERAUTH_BANNER:
			break;
		}
	} else if (c->state == Established) {
		if (established(c, p, p2, buf, sizeof buf) < 0)
			bail(c, p, p2, "from established state");
	} else {
		sshdebug(c, "connection %d in bad state, reader exiting", c->id);
		bail(c, p, p2, "bad conn state");
	}
}

void
reader(void *a)
{
	Conn *c;
	Packet *p, *p2;

	threadsetname("reader");
	c = a;
	c->rpid = threadid();
	sshdebug(c, "starting reader for connection %d, pid %d", c->id, c->rpid);
	threadsetname("reader");
	p = new_packet(c);
	p2 = new_packet(c);
	c->rio = ioproc();
	for(;;)
		reader0(c, p, p2);
}

int
validatekex(Conn *c, Packet *p)
{
	if (c->role == Server)
		return validatekexs(p);
	else
		return validatekexc(p);
}

int
validatekexs(Packet *p)
{
	uchar *q;
	char *toks[Maxtoks];
	int i, j, n;
	char *buf;

	buf = emalloc9p(Bigbufsz);
	q = p->payload + 17;

	q = get_string(p, q, buf, Bigbufsz, nil);
	sshdebug(nil, "received KEX algs: %s", buf);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (i = 0; i < n; ++i)
		for (j = 0; j < nelem(kexes); ++j)
			if (strcmp(toks[i], kexes[j]->name) == 0)
				goto foundk;
	sshdebug(nil, "kex algs not in kexes");
	free(buf);
	return -1;
foundk:
	p->c->kexalg = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	sshdebug(nil, "received host key algs: %s", buf);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (i = 0; i < n; ++i)
		for (j = 0; j < nelem(pkas) && pkas[j] != nil; ++j)
			if (strcmp(toks[i], pkas[j]->name) == 0)
				goto foundpka;
	sshdebug(nil, "host key algs not in pkas");
	free(buf);
	return -1;
foundpka:
	p->c->pkalg = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	sshdebug(nil, "received C2S crypto algs: %s", buf);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (i = 0; i < n; ++i)
		for (j = 0; j < nelem(cryptos); ++j)
			if (strcmp(toks[i], cryptos[j]->name) == 0)
				goto foundc1;
	sshdebug(nil, "c2s crypto algs not in cryptos");
	free(buf);
	return -1;
foundc1:
	p->c->ncscrypt = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	sshdebug(nil, "received S2C crypto algs: %s", buf);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (i = 0; i < n; ++i)
		for (j = 0; j < nelem(cryptos); ++j)
			if (strcmp(toks[i], cryptos[j]->name) == 0)
				goto foundc2;
	sshdebug(nil, "s2c crypto algs not in cryptos");
	free(buf);
	return -1;
foundc2:
	p->c->nsccrypt = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	sshdebug(nil, "received C2S MAC algs: %s", buf);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (i = 0; i < n; ++i)
		for (j = 0; j < nelem(macnames); ++j)
			if (strcmp(toks[i], macnames[j]) == 0)
				goto foundm1;
	sshdebug(nil, "c2s mac algs not in cryptos");
	free(buf);
	return -1;
foundm1:
	p->c->ncsmac = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	sshdebug(nil, "received S2C MAC algs: %s", buf);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (i = 0; i < n; ++i)
		for (j = 0; j < nelem(macnames); ++j)
			if (strcmp(toks[i], macnames[j]) == 0)
				goto foundm2;
	sshdebug(nil, "s2c mac algs not in cryptos");
	free(buf);
	return -1;
foundm2:
	p->c->nscmac = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	q = get_string(p, q, buf, Bigbufsz, nil);
	q = get_string(p, q, buf, Bigbufsz, nil);
	q = get_string(p, q, buf, Bigbufsz, nil);
	free(buf);
	if (*q)
		return 1;
	return 0;
}

int
validatekexc(Packet *p)
{
	uchar *q;
	char *toks[Maxtoks];
	int i, j, n;
	char *buf;

	buf = emalloc9p(Bigbufsz);
	q = p->payload + 17;
	q = get_string(p, q, buf, Bigbufsz, nil);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (j = 0; j < nelem(kexes); ++j)
		for (i = 0; i < n; ++i)
			if (strcmp(toks[i], kexes[j]->name) == 0)
				goto foundk;
	free(buf);
	return -1;
foundk:
	p->c->kexalg = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (j = 0; j < nelem(pkas) && pkas[j] != nil; ++j)
		for (i = 0; i < n; ++i)
			if (strcmp(toks[i], pkas[j]->name) == 0)
				goto foundpka;
	free(buf);
	return -1;
foundpka:
	p->c->pkalg = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (j = 0; j < nelem(cryptos); ++j)
		for (i = 0; i < n; ++i)
			if (strcmp(toks[i], cryptos[j]->name) == 0)
				goto foundc1;
	free(buf);
	return -1;
foundc1:
	p->c->ncscrypt = j;
	q = get_string(p, q, buf, Bigbufsz, nil);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (j = 0; j < nelem(cryptos); ++j)
		for (i = 0; i < n; ++i)
			if (strcmp(toks[i], cryptos[j]->name) == 0)
				goto foundc2;
	free(buf);
	return -1;
foundc2:
	p->c->nsccrypt = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (j = 0; j < nelem(macnames); ++j)
		for (i = 0; i < n; ++i)
			if (strcmp(toks[i], macnames[j]) == 0)
				goto foundm1;
	free(buf);
	return -1;
foundm1:
	p->c->ncsmac = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	n = gettokens(buf, toks, nelem(toks), ",");
	for (j = 0; j < nelem(macnames); ++j)
		for (i = 0; i < n; ++i)
			if (strcmp(toks[i], macnames[j]) == 0)
				goto foundm2;
	free(buf);
	return -1;
foundm2:
	p->c->nscmac = j;

	q = get_string(p, q, buf, Bigbufsz, nil);
	q = get_string(p, q, buf, Bigbufsz, nil);
	q = get_string(p, q, buf, Bigbufsz, nil);
	q = get_string(p, q, buf, Bigbufsz, nil);
	free(buf);
	return *q != 0;
}

int
memrandom(void *p, int n)
{
	uchar *cp;

	for (cp = (uchar*)p; n > 0; n--)
		*cp++ = fastrand();
	return 0;
}

/*
 *  create a change uid capability
 */
char*
mkcap(char *from, char *to)
{
	int fd, fromtosz;
	char *cap, *key;
	uchar rand[SHA1dlen], hash[SHA1dlen];

	fd = open("#¤/caphash", OWRITE);
	if (fd < 0)
		sshlog(nil, "can't open #¤/caphash: %r");

	/* create the capability */
	fromtosz = strlen(from) + 1 + strlen(to) + 1;
	cap = emalloc9p(fromtosz + sizeof(rand)*3 + 1);
	snprint(cap, fromtosz + sizeof(rand)*3 + 1, "%s@%s", from, to);
	memrandom(rand, sizeof(rand));
	key = cap + fromtosz;
	enc64(key, sizeof(rand)*3, rand, sizeof(rand));

	/* hash the capability */
	hmac_sha1((uchar*)cap, strlen(cap), (uchar*)key, strlen(key), hash, nil);

	/* give the kernel the hash */
	key[-1] = '@';
	sshdebug(nil, "writing `%.*s' to caphash", SHA1dlen, hash);
	if (write(fd, hash, SHA1dlen) != SHA1dlen) {
		close(fd);
		free(cap);
		return nil;
	}
	close(fd);
	return cap;
}

/*
 * ask keyfs (assumes we are on an auth server)
 */
static AuthInfo *
keyfsauth(char *me, char *user, char *pw, char *key1, char *key2)
{
	int fd;
	char path[Arbpathlen];
	AuthInfo *ai;

	if (passtokey(key1, pw) == 0)
		return nil;

	snprint(path, Arbpathlen, "/mnt/keys/%s/key", user);
	if ((fd = open(path, OREAD)) < 0) {
		werrstr("Invalid user %s", user);
		return nil;
	}
	if (read(fd, key2, DESKEYLEN) != DESKEYLEN) {
		close(fd);
		werrstr("Password mismatch 1");
		return nil;
	}
	close(fd);

	if (memcmp(key1, key2, DESKEYLEN) != 0) {
		werrstr("Password mismatch 2");
		return nil;
	}

	ai = emalloc9p(sizeof(AuthInfo));
	ai->cuid = estrdup9p(user);
	ai->suid = estrdup9p(me);
	ai->cap = mkcap(me, user);
	ai->nsecret = 0;
	ai->secret = (uchar *)estrdup9p("");
	return ai;
}

static void
userauthfailed(Packet *p2)
{
	add_byte(p2, SSH_MSG_USERAUTH_FAILURE);
	add_string(p2, "password,publickey");
	add_byte(p2, 0);
}

static int
authreqpk(Packet *p, Packet *p2, Conn *c, char *user, uchar *q,
	char *alg, char *blob, char *sig, char *service, char *me)
{
	int n, thisway, nblob, nsig;
	char method[32];

	sshdebug(c, "auth_req publickey for user %s", user);
	thisway = *q == '\0';
	q = get_string(p, q+1, alg, Arbpathlen, nil);
	q = get_string(p, q, blob, Blobsz, &nblob);
	if (thisway) {
		/*
		 * Should really check to see if this user can
		 * be authed this way.
		 */
		for (n = 0; n < nelem(pkas) && pkas[n] != nil &&
		    strcmp(pkas[n]->name, alg) != 0; ++n)
			;
		if (n >= nelem(pkas) || pkas[n] == nil) {
			userauthfailed(p2);
			return -1;
		}
		add_byte(p2, SSH_MSG_USERAUTH_PK_OK);
		add_string(p2, alg);
		add_block(p2, blob, nblob);
		return 1;
	}

	get_string(p, q, sig, Blobsz, &nsig);
	for (n = 0; n < nelem(pkas) && pkas[n] != nil &&
	    strcmp(pkas[n]->name, alg) != 0; ++n)
		;
	if (n >= nelem(pkas) || pkas[n] == nil) {
		userauthfailed(p2);
		return -1;
	}

	add_block(p2, c->sessid, SHA1dlen);
	add_byte(p2, SSH_MSG_USERAUTH_REQUEST);
	add_string(p2, user);
	add_string(p2, service);
	add_string(p2, method);
	add_byte(p2, 1);
	add_string(p2, alg);
	add_block(p2, blob, nblob);
	if (pkas[n]->verify(c, p2->payload, p2->rlength - 1, user, sig, nsig)
	    == 0) {
		init_packet(p2);
		p2->c = c;
		sshlog(c, "public key login failed");
		userauthfailed(p2);
		return -1;
	}
	free(c->cap);
	c->cap = mkcap(me, user);
	init_packet(p2);
	p2->c = c;
	sshlog(c, "logged in by public key");
	add_byte(p2, SSH_MSG_USERAUTH_SUCCESS);
	return 0;
}

int
auth_req(Packet *p, Conn *c)
{
	int n, ret;
	char *alg, *blob, *sig, *service, *me, *user, *pw, *path;
	char key1[DESKEYLEN], key2[DESKEYLEN], method[32];
	uchar *q;
	AuthInfo *ai;
	Packet *p2;

	service = emalloc9p(Arbpathlen);
	me = emalloc9p(Arbpathlen);
	user = emalloc9p(Arbpathlen);
	pw = emalloc9p(Arbpathlen);
	alg = emalloc9p(Arbpathlen);
	path = emalloc9p(Arbpathlen);
	blob = emalloc9p(Blobsz);
	sig = emalloc9p(Blobsz);
	ret = -1;				/* failure is default */

	q = get_string(p, p->payload + 1, user, Arbpathlen, nil);
	free(c->user);
	c->user = estrdup9p(user);
	q = get_string(p, q, service, Arbpathlen, nil);
	q = get_string(p, q, method, sizeof method, nil);
	sshdebug(c, "got userauth request: %s %s %s", user, service, method);

	readfile("/dev/user", me, Arbpathlen);

	p2 = new_packet(c);
	if (strcmp(method, "publickey") == 0)
		ret = authreqpk(p, p2, c, user, q, alg, blob, sig, service, me);
	else if (strcmp(method, "password") == 0) {
		get_string(p, q + 1, pw, Arbpathlen, nil);
		// sshdebug(c, "%s", pw);	/* bad idea to log passwords */
		sshdebug(c, "auth_req password");
		if (kflag)
			ai = keyfsauth(me, user, pw, key1, key2);
		else
			ai = auth_userpasswd(user, pw);
		if (ai == nil) {
			sshlog(c, "login failed: %r");
			userauthfailed(p2);
		} else {
			sshdebug(c, "auth successful: cuid %s suid %s cap %s",
				ai->cuid, ai->suid, ai->cap);
			free(c->cap);
			if (strcmp(user, me) == 0)
				c->cap = estrdup9p("n/a");
			else
				c->cap = estrdup9p(ai->cap);
			sshlog(c, "logged in by password");
			add_byte(p2, SSH_MSG_USERAUTH_SUCCESS);
			auth_freeAI(ai);
			ret = 0;
		}
	} else
		userauthfailed(p2);

	n = finish_packet(p2);
	iowrite(c->dio, c->datafd, p2->nlength, n);

	free(service);
	free(me);
	free(user);
	free(pw);
	free(alg);
	free(blob);
	free(sig);
	free(path);
	free(p2);
	return ret;
}

int
client_auth(Conn *c, Ioproc *io)
{
	Packet *p2, *p3, *p4;
	char *r, *s;
	mpint *ek, *nk;
	int i, n;

	sshdebug(c, "client_auth");
	if (!c->password && !c->authkey)
		return -1;

	p2 = new_packet(c);
	add_byte(p2, SSH_MSG_USERAUTH_REQUEST);
	add_string(p2, c->user);
	add_string(p2, c->service);
	if (c->password) {
		add_string(p2, "password");
		add_byte(p2, 0);
		add_string(p2, c->password);
		sshdebug(c, "client_auth using password for svc %s", c->service);
	} else {
		sshdebug(c, "client_auth trying rsa public key");
		add_string(p2, "publickey");
		add_byte(p2, 1);
		add_string(p2, "ssh-rsa");

		r = strstr(c->authkey, " ek=");
		s = strstr(c->authkey, " n=");
		if (!r || !s) {
			shutdown(c);
			free(p2);
			sshdebug(c, "client_auth no rsa key");
			return -1;
		}
		ek = strtomp(r+4, nil, 16, nil);
		nk = strtomp(s+3, nil, 16, nil);

		p3 = new_packet(c);
		add_string(p3, "ssh-rsa");
		add_mp(p3, ek);
		add_mp(p3, nk);
		add_block(p2, p3->payload, p3->rlength-1);

		p4 = new_packet(c);
		add_block(p4, c->sessid, SHA1dlen);
		add_byte(p4, SSH_MSG_USERAUTH_REQUEST);
		add_string(p4, c->user);
		add_string(p4, c->service);
		add_string(p4, "publickey");
		add_byte(p4, 1);
		add_string(p4, "ssh-rsa");
		add_block(p4, p3->payload, p3->rlength-1);
		mpfree(ek);
		mpfree(nk);
		free(p3);

		for (i = 0; pkas[i] && strcmp("ssh-rsa", pkas[i]->name) != 0;
		    ++i)
			;
		sshdebug(c, "client_auth rsa signing alg %d: %r",  i);
		if ((p3 = pkas[i]->sign(c, p4->payload, p4->rlength-1)) == nil) {
			sshdebug(c, "client_auth rsa signing failed: %r");
			free(p4);
			free(p2);
			return -1;
		}
		add_block(p2, p3->payload, p3->rlength-1);
		free(p3);
		free(p4);
	}

	n = finish_packet(p2);
	if (writeio(io, c->datafd, p2->nlength, n) != n)
		sshdebug(c, "client_auth write failed: %r");
	free(p2);
	return 0;
}

/* should use auth_getkey or something similar */
char *
factlookup(int nattr, int nreq, char *attrs[])
{
	Biobuf *bp;
	char *buf, *toks[Maxtoks], *res, *q;
	int ntok, nmatch, maxmatch;
	int i, j;

	res = nil;
	bp = Bopen("/mnt/factotum/ctl", OREAD);
	if (bp == nil)
		return nil;
	maxmatch = 0;
	while (buf = Brdstr(bp, '\n', 1)) {
		q = estrdup9p(buf);
		ntok = gettokens(buf, toks, nelem(toks), " ");
		nmatch = 0;
		for (i = 0; i < nattr; ++i) {
			for (j = 0; j < ntok; ++j)
				if (strcmp(attrs[i], toks[j]) == 0) {
					++nmatch;
					break;
				}
			if (i < nreq && j >= ntok)
				break;
		}
		if (i >= nattr && nmatch > maxmatch) {
			free(res);
			res = q;
			maxmatch = nmatch;
		} else
			free(q);
		free(buf);
	}
	Bterm(bp);
	return res;
}

void
shutdown(Conn *c)
{
	Plist *p;
	SSHChan *sc;
	int i, ostate;

	sshdebug(c, "shutting down connection %d", c->id);
	ostate = c->state;
	if (c->clonefile->ref <= 2 && c->ctlfile->ref <= 2 &&
	    c->datafile->ref <= 2 && c->listenfile->ref <= 2 &&
	    c->localfile->ref <= 2 && c->remotefile->ref <= 2 &&
	    c->statusfile->ref <= 2)
		c->state = Closed;
	else {
		if (c->state != Closed)
			c->state = Closing;
		sshdebug(c, "clone %ld ctl %ld data %ld listen %ld "
			"local %ld remote %ld status %ld",
			c->clonefile->ref, c->ctlfile->ref, c->datafile->ref,
			c->listenfile->ref, c->localfile->ref, c->remotefile->ref,
			c->statusfile->ref);
	}
	if (ostate == Closed || ostate == Closing) {
		c->state = Closed;
		return;
	}
	if (c->role == Server && c->remote)
		sshlog(c, "closing connection");
	hangupconn(c);
	if (c->dio) {
		closeioproc(c->dio);
		c->dio = nil;
	}

	c->decrypt = -1;
	c->inmac = -1;
	c->nchan = 0;
	free(c->otherid);
	free(c->s2ccs);
	c->s2ccs = nil;
	free(c->c2scs);
	c->c2scs = nil;
	free(c->remote);
	c->remote = nil;
	if (c->x) {
		mpfree(c->x);
		c->x = nil;
	}
	if (c->e) {
		mpfree(c->e);
		c->e = nil;
	}
	free(c->user);
	c->user = nil;
	free(c->service);
	c->service = nil;
	c->otherid = nil;
	qlock(&c->l);
	rwakeupall(&c->r);
	qunlock(&c->l);
	for (i = 0; i < MAXCONN; ++i) {
		sc = c->chans[i];
		if (sc == nil)
			continue;
		free(sc->ann);
		sc->ann = nil;
		if (sc->state != Empty && sc->state != Closed) {
			sc->state = Closed;
			sc->lreq = nil;
			while (sc->dataq != nil) {
				p = sc->dataq;
				sc->dataq = p->next;
				free(p->pack);
				free(p);
			}
			while (sc->reqq != nil) {
				p = sc->reqq;
				sc->reqq = p->next;
				free(p->pack);
				free(p);
			}
			qlock(&c->l);
			rwakeupall(&sc->r);
			nbsendul(sc->inchan, 1);
			nbsendul(sc->reqchan, 1);
			chanclose(sc->inchan);
			chanclose(sc->reqchan);
			qunlock(&c->l);
		}
	}
	qlock(&availlck);
	rwakeup(&availrend);
	qunlock(&availlck);
	sshdebug(c, "done processing shutdown of connection %d", c->id);
}
