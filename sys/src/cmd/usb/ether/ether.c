/*
 * usb/ether - usb ethernet adapter.
 * BUG: This should use /dev/etherfile to
 * use the kernel ether device code.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ether.h"

typedef struct Dirtab Dirtab;

enum
{
	/* Qids. Maintain order (relative to dirtabs structs) */
	Qroot	= 0,
	Qclone,
	Qaddr,
	Qifstats,
	Qstats,
	Qndir,
	Qndata,
	Qnctl,
	Qnifstats,
	Qnstats,
	Qntype,
	Qmax,
};

struct Dirtab
{
	char	*name;
	int	qid;
	int	mode;
};

typedef int (*Resetf)(Ether*);

/*
 * Controllers by vid/vid. Used to locate
 * specific adapters that do not implement cdc ethernet
 * Keep null terminated.
 */
Cinfo cinfo[] =
{
	/* Asix controllers.
	 * Only A88178 and A881772 are implemented.
	 * Others are easy to add by borrowing code
	 * from other systems.
	 */
	{0x077b, 0x2226, A8817x},
	{0x0b95, 0x1720, A8817x},
	{0x0557, 0x2009, A8817x},
	{0x0411, 0x003d, A8817x},
	{0x0411, 0x006e, A88178},
	{0x6189, 0x182d, A8817x},
	{0x07aa, 0x0017, A8817x},
	{0x1189, 0x0893, A8817x},
	{0x1631, 0x6200, A8817x},
	{0x04f1, 0x3008, A8817x},
	{0x0b95, 0x1780, A88178},	/* Geoff */
	{0x13b1, 0x0018, A88772},
	{0x1557, 0x7720, A88772},
	{0x07d1, 0x3c05, A88772},
	{0x2001, 0x3c05, A88772},
	{0x1737, 0x0039, A88178},
	{0x050d, 0x5055, A88178},
	{0x05ac, 0x1402, A88772},	/* Apple */
	{0x0b95, 0x772a, A88772},
	{0x14ea, 0xab11, A88178},
	{0x0db0, 0xa877, A88772},
	{0, 0, 0},
};

/*
 * Each etherU%d is the root of our file system,
 * which is added to the usb root directory. We only
 * have to concern ourselfs with each /etherU%d subtree.
 *
 * NB: Maintain order in dirtabs, relative to the Qids enum.
 */

static Dirtab rootdirtab[] =
{
	"/",		Qroot,		DMDIR|0555,	/* etherU%d */
	"clone",	Qclone,		0666,
	"addr",		Qaddr,		0444,
	"ifstats",	Qifstats,	0444,
	"stats",	Qstats,		0444,
	/* one dir per connection here */
	nil, 0, 0,
};

static Dirtab conndirtab[] =
{
	"%d",		Qndir,		DMDIR|0555,
	"data",		Qndata,		0666,
	"ctl",		Qnctl,		0666,
	"ifstats",	Qnifstats,	0444,
	"stats",	Qnstats,	0444,
	"type",		Qntype,		0444,
	nil, 0,
};

int etherdebug;

Resetf ethers[] =
{
	asixreset,
	cdcreset,	/* keep last */
};

static int
qtype(vlong q)
{
	return q&0xFF;
}

static int
qnum(vlong q)
{
	return (q >> 8) & 0xFFFFFF;
}

static uvlong
mkqid(int n, int t)
{
	uvlong q;

	q = (n&0xFFFFFF) << 8 | t&0xFF;
	return q;
}

static void
freebuf(Ether *e, Buf *bp)
{
	if(0)deprint(2, "%s: freebuf %#p\n", argv0, bp);
	if(bp != nil){
		qlock(e);
		e->nbufs--;
		qunlock(e);
		sendp(e->bc, bp);
	}
}

static Buf*
allocbuf(Ether *e)
{
	Buf *bp;

	bp = nbrecvp(e->bc);
	if(bp == nil){
		qlock(e);
		if(e->nabufs < Nconns){
			bp = emallocz(sizeof(Buf), 1);
			e->nabufs++;
			setmalloctag(bp, getcallerpc(&e));
			deprint(2, "%s: %d buffers\n", argv0, e->nabufs);
		}
		qunlock(e);
	}
	if(bp == nil)
		bp = recvp(e->bc);
	bp->rp = bp->data + Hdrsize;
	bp->ndata = 0;
	if(0)deprint(2, "%s: allocbuf %#p\n", argv0, bp);
	qlock(e);
	e->nbufs++;
	qunlock(e);
	return bp;
}

static Conn*
newconn(Ether *e)
{
	int i;
	Conn *c;

	qlock(e);
	for(i = 0; i < nelem(e->conns); i++){
		c = e->conns[i];
		if(c == nil || c->ref == 0){
			if(c == nil){
				c = emallocz(sizeof(Conn), 1);
				c->rc = chancreate(sizeof(Buf*), 2);
				c->nb = i;
			}
			c->ref = 1;
			if(i == e->nconns)
				e->nconns++;
			e->conns[i] = c;
			deprint(2, "%s: newconn %d\n", argv0, i);
			qunlock(e);
			return c;
		}
	}
	qunlock(e);
	return nil;
}

static char*
seprintaddr(char *s, char *se, uchar *addr)
{
	int i;

	for(i = 0; i < Eaddrlen; i++)
		s = seprint(s, se, "%02x", addr[i]);
	return s;
}

void
dumpframe(char *tag, void *p, int n)
{
	Etherpkt *ep;
	char buf[128];
	char *s, *se;
	int i;

	ep = p;
	if(n < Eaddrlen * 2 + 2){
		fprint(2, "short packet (%d bytes)\n", n);
		return;
	}
	se = buf+sizeof(buf);
	s = seprint(buf, se, "%s [%d]: ", tag, n);
	s = seprintaddr(s, se, ep->s);
	s = seprint(s, se, " -> ");
	s = seprintaddr(s, se, ep->d);
	s = seprint(s, se, " type 0x%02ux%02ux ", ep->type[0], ep->type[1]);
	n -= Eaddrlen * 2 + 2;
	for(i = 0; i < n && i < 16; i++)
		s = seprint(s, se, "%02x", ep->data[i]);
	if(n >= 16)
		fprint(2, "%s...\n", buf);
	else
		fprint(2, "%s\n", buf);
}

static char*
seprintstats(char *s, char *se, Ether *e)
{
	qlock(e);
	s = seprint(s, se, "in: %ld\n", e->nin);
	s = seprint(s, se, "out: %ld\n", e->nout);
	s = seprint(s, se, "input errs: %ld\n", e->nierrs);
	s = seprint(s, se, "output errs: %ld\n", e->noerrs);
	s = seprint(s, se, "mbps: %d\n", e->mbps);
	s = seprint(s, se, "prom: %ld\n", e->prom.ref);
	s = seprint(s, se, "addr: ");
	s = seprintaddr(s, se, e->addr);
	s = seprint(s, se, "\n");
	qunlock(e);
	return s;
}

static char*
seprintifstats(char *s, char *se, Ether *e)
{
	int i;
	Conn *c;

	qlock(e);
	s = seprint(s, se, "ctlr id: %#x\n", e->cid);
	s = seprint(s, se, "phy: %#x\n", e->phy);
	s = seprint(s, se, "exiting: %s\n", e->exiting ? "y" : "n");
	s = seprint(s, se, "conns: %d\n", e->nconns);
	s = seprint(s, se, "allocated bufs: %d\n", e->nabufs);
	s = seprint(s, se, "used bufs: %d\n", e->nbufs);
	for(i = 0; i < nelem(e->conns); i++){
		c = e->conns[i];
		if(c == nil)
			continue;
		if(c->ref == 0)
			s = seprint(s, se, "c[%d]: free\n", i);
		else{
			s = seprint(s, se, "c[%d]: refs %ld t %#x h %d p %d\n",
				c->nb, c->ref, c->type, c->headersonly, c->prom);
		}
	}
	qunlock(e);
	return s;
}

static void
etherdump(Ether *e)
{
	char buf[256];

	if(etherdebug == 0)
		return;
	seprintifstats(buf, buf+sizeof(buf), e);
	fprint(2, "%s: ether %#p:\n%s\n", argv0, e, buf);
}

static Conn*
getconn(Ether *e, int i, int idleok)
{
	Conn *c;

	qlock(e);
	if(i < 0 || i >= e->nconns)
		c = nil;
	else{
		c = e->conns[i];
		if(idleok == 0 && c != nil && c->ref == 0)
			c = nil;
	}
	qunlock(e);
	return c;
}

static void
filldir(Usbfs *fs, Dir *d, Dirtab *tab, int cn)
{
	d->qid.path = mkqid(cn, tab->qid);
	d->qid.path |= fs->qid;
	d->mode = tab->mode;
	if((d->mode & DMDIR) != 0)
		d->qid.type = QTDIR;
	else
		d->qid.type = QTFILE;
	if(tab->qid == Qndir)
		snprint(d->name, Namesz, "%d", cn);
	else
		d->name = tab->name;
}

static int
rootdirgen(Usbfs *fs, Qid, int i, Dir *d, void *)
{
	Ether *e;
	Dirtab *tab;
	int cn;

	e = fs->aux;
	i++;				/* skip root */
	cn = 0;
	if(i < nelem(rootdirtab) - 1)	/* null terminated */
		tab = &rootdirtab[i];
	else{
		cn = i - nelem(rootdirtab) + 1;
		if(cn < e->nconns)
			tab = &conndirtab[0];
		else
			return -1;
	}
	filldir(fs, d, tab, cn);
	return 0;
}

static int
conndirgen(Usbfs *fs, Qid q, int i, Dir *d, void *)
{
	Dirtab *tab;

	i++;				/* skip root */
	if(i < nelem(conndirtab) - 1)	/* null terminated */
		tab = &conndirtab[i];
	else
		return -1;
	filldir(fs, d, tab, qnum(q.path));
	return 0;
}

static int
fswalk(Usbfs *fs, Fid *fid, char *name)
{
	int cn, i;
	char *es;
	Dirtab *tab;
	Ether *e;
	Qid qid;

	e = fs->aux;
	qid = fid->qid;
	qid.path &= ~fs->qid;
	if((qid.type & QTDIR) == 0){
		werrstr("walk in non-directory");
		return -1;
	}

	if(strcmp(name, "..") == 0){
		/* must be /etherU%d; i.e. our root dir. */
		fid->qid.path = mkqid(0, Qroot) | fs->qid;
		fid->qid.vers = 0;
		fid->qid.type = QTDIR;
		return 0;
	}
	switch(qtype(qid.path)){
	case Qroot:
		if(name[0] >= '0' && name[0] <= '9'){
			es = name;
			cn = strtoul(name, &es, 10);
			if(cn >= e->nconns || *es != 0){
				werrstr(Enotfound);
				return -1;
			}
			fid->qid.path = mkqid(cn, Qndir) | fs->qid;
			fid->qid.vers = 0;
			return 0;
		}
		/* fall */
	case Qndir:
		if(qtype(qid.path) == Qroot)
			tab = rootdirtab;
		else
			tab = conndirtab;
		cn = qnum(qid.path);
		for(i = 0; tab[i].name != nil; tab++)
			if(strcmp(tab[i].name, name) == 0){
				fid->qid.path = mkqid(cn, tab[i].qid)|fs->qid;
				fid->qid.vers = 0;
				if((tab[i].mode & DMDIR) != 0)
					fid->qid.type = QTDIR;
				else
					fid->qid.type = QTFILE;
				return 0;
			}
		break;
	default:
		sysfatal("usb: ether: fswalk bug");
	}
	return -1;
}

static Dirtab*
qdirtab(vlong q)
{
	int i, qt;
	Dirtab *tab;

	qt = qtype(q);
	if(qt < nelem(rootdirtab) - 1){	/* null terminated */
		tab = rootdirtab;
		i = qt;
	}else{
		tab = conndirtab;
		i = qt - (nelem(rootdirtab) - 1);
		assert(i < nelem(conndirtab) - 1);
	}
	return &tab[i];
}

static int
fsstat(Usbfs *fs, Qid qid, Dir *d)
{
	filldir(fs, d, qdirtab(qid.path), qnum(qid.path));
	return 0;
}

static int
fsopen(Usbfs *fs, Fid *fid, int omode)
{
	int qt;
	vlong qid;
	Conn *c;
	Dirtab *tab;
	Ether *e;

	qid = fid->qid.path & ~fs->qid;
	e = fs->aux;
	qt = qtype(qid);
	tab = qdirtab(qid);
	omode &= 3;
	if(omode != OREAD && (tab->mode&0222) == 0){
		werrstr(Eperm);
		return -1;
	}
	switch(qt){
	case Qclone:
		c = newconn(e);
		if(c == nil){
			werrstr("no more connections");
			return -1;
		}
		fid->qid.type = QTFILE;
		fid->qid.path = mkqid(c->nb, Qnctl)|fs->qid;
		fid->qid.vers = 0;
		break;
	case Qndata:
	case Qnctl:
	case Qnifstats:
	case Qnstats:
	case Qntype:
		c = getconn(e, qnum(qid), 1);
		if(c == nil)
			sysfatal("usb: ether: fsopen bug");
		incref(c);
		break;
	}
	etherdump(e);
	return 0;
}

static int
prom(Ether *e, int set)
{
	if(e->promiscuous != nil)
		return e->promiscuous(e, set);
	return 0;
}

static void
fsclunk(Usbfs *fs, Fid *fid)
{
	int qt;
	vlong qid;
	Buf *bp;
	Conn *c;
	Ether *e;

	e = fs->aux;
	qid = fid->qid.path & ~fs->qid;
	qt = qtype(qid);
	switch(qt){
	case Qndata:
	case Qnctl:
	case Qnifstats:
	case Qnstats:
	case Qntype:
		if(fid->omode != ONONE){
			c = getconn(e, qnum(qid), 0);
			if(c == nil)
				sysfatal("usb: ether: fsopen bug");
			if(decref(c) == 0){
				while((bp = nbrecvp(c->rc)) != nil)
					freebuf(e, bp);
				qlock(e);
				if(c->prom != 0)
					if(decref(&e->prom) == 0)
						prom(e, 0);
				c->prom = c->type = 0;
				qunlock(e);
			}
		}
		break;
	}
	etherdump(e);
}

int
parseaddr(uchar *m, char *s)
{
	int i, n;
	uchar v;

	if(strlen(s) < 12)
		return -1;
	if(strlen(s) > 12 && strlen(s) < 17)
		return -1;
	for(i = n = 0; i < strlen(s); i++){
		if(s[i] == ':')
			continue;
		if(s[i] >= 'A' && s[i] <= 'F')
			v = 10 + s[i] - 'A';
		else if(s[i] >= 'a' && s[i] <= 'f')
			v = 10 + s[i] - 'a';
		else if(s[i] >= '0' && s[i] <= '9')
			v = s[i] - '0';
		else
			return -1;
		if(n&1)
			m[n/2] |= v;
		else
			m[n/2] = v<<4;
		n++;
	}
	return 0;
}

static long
fsread(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	int cn, qt;
	char *s, *se;
	char buf[2048];		/* keep this large for ifstats */
	Buf *bp;
	Conn *c;
	Ether *e;
	Qid q;

	q = fid->qid;
	q.path &= ~fs->qid;
	e = fs->aux;
	s = buf;
	se = buf+sizeof(buf);
	qt = qtype(q.path);
	cn = qnum(q.path);
	switch(qt){
	case Qroot:
		count = usbdirread(fs, q, data, count, offset, rootdirgen, nil);
		break;
	case Qaddr:
		s = seprintaddr(s, se, e->addr);
		count = usbreadbuf(data, count, offset, buf, s - buf);
		break;
	case Qnifstats:
		/* BUG */
	case Qifstats:
		s = seprintifstats(s, se, e);
		if(e->seprintstats != nil)
			s = e->seprintstats(s, se, e);
		count = usbreadbuf(data, count, offset, buf, s - buf);
		break;
	case Qnstats:
		/* BUG */
	case Qstats:
		s = seprintstats(s, se, e);
		count = usbreadbuf(data, count, offset, buf, s - buf);
		break;

	case Qndir:
		count = usbdirread(fs, q, data, count, offset, conndirgen, nil);
		break;
	case Qndata:
		c = getconn(e, cn, 0);
		if(c == nil){
			werrstr(Eio);
			return -1;
		}
		bp = recvp(c->rc);
		if(bp == nil)
			return -1;
		if(etherdebug > 1)
			dumpframe("etherin", bp->rp, bp->ndata);
		count = usbreadbuf(data, count, 0LL, bp->rp, bp->ndata);
		freebuf(e, bp);
		break;
	case Qnctl:
		s = seprint(s, se, "%11d ", cn);
		count = usbreadbuf(data, count, offset, buf, s - buf);
		break;
	case Qntype:
		c = getconn(e, cn, 0);
		if(c == nil)
			s = seprint(s, se, "%11d ", 0);
		else
			s = seprint(s, se, "%11d ", c->type);
		count = usbreadbuf(data, count, offset, buf, s - buf);
		break;
	default:
		sysfatal("usb: ether: fsread bug");
	}
	return count;
}

static int
typeinuse(Ether *e, int t)
{
	int i;

	for(i = 0; i < e->nconns; i++)
		if(e->conns[i]->ref > 0 && e->conns[i]->type == t)
			return 1;
	return 0;
}

static int
isloopback(Ether *e, Buf *)
{
	return e->prom.ref > 0; /* BUG: also loopbacks and broadcasts */
}

static int
etherctl(Ether *e, Conn *c, char *buf)
{
	uchar addr[Eaddrlen];
	int t;

	deprint(2, "%s: etherctl: %s\n", argv0, buf);
	if(strncmp(buf, "connect ", 8) == 0){
		t = atoi(buf+8);
		qlock(e);
		if(typeinuse(e, t)){
			werrstr("type already in use");
			qunlock(e);
			return -1;
		}
		c->type = atoi(buf+8);
		qunlock(e);
		return 0;
	}
	if(strncmp(buf, "nonblocking", 11) == 0){
		if(buf[11] == '\n' || buf[11] == 0)
			e->nblock = 1;
		else
			e->nblock = atoi(buf + 12);
		deprint(2, "%s: nblock %d\n", argv0, e->nblock);
		return 0;
	}
	if(strncmp(buf, "promiscuous", 11) == 0){
		if(c->prom == 0)
			incref(&e->prom);
		c->prom = 1;
		return prom(e, 1);
	}
	if(strncmp(buf, "headersonly", 11) == 0){
		c->headersonly = 1;
		return 0;
	}
	if(strncmp(buf, "addmulti ", 9) == 0 || strncmp(buf, "remmulti ", 9) == 0){
		if(parseaddr(addr, buf+9) < 0){
			werrstr("bad address");
			return -1;
		}
		if(e->multicast == nil)
			return 0;
		if(strncmp(buf, "add", 3) == 0){
			e->nmcasts++;
			return e->multicast(e, addr, 1);
		}else{
			e->nmcasts--;
			return e->multicast(e, addr, 0);
		}
	}

	if(e->ctl != nil)
		return e->ctl(e, buf);
	werrstr(Ebadctl);
	return -1;
}

static long
etherbread(Ether *e, Buf *bp)
{
	deprint(2, "%s: etherbread\n", argv0);
	bp->rp = bp->data + Hdrsize;
	bp->ndata = -1;
	bp->ndata = read(e->epin->dfd, bp->rp, sizeof(bp->data)-Hdrsize);
	if(bp->ndata < 0){
		deprint(2, "%s: etherbread: %r\n", argv0);	/* keep { and }  */
	}else
		deprint(2, "%s: etherbread: got %d bytes\n", argv0, bp->ndata);
	return bp->ndata;
}

static long
etherbwrite(Ether *e, Buf *bp)
{
	long n;

	deprint(2, "%s: etherbwrite %d bytes\n", argv0, bp->ndata);
	n = write(e->epout->dfd, bp->rp, bp->ndata);
	if(n < 0){
		deprint(2, "%s: etherbwrite: %r\n", argv0);	/* keep { and }  */
	}else
		deprint(2, "%s: etherbwrite wrote %ld bytes\n", argv0, n);
	if(n <= 0)
		return n;
	if((bp->ndata % e->epout->maxpkt) == 0){
		deprint(2, "%s: short pkt write\n", argv0);
		write(e->epout->dfd, "", 1);
	}
	return n;
}

static long
fswrite(Usbfs *fs, Fid *fid, void *data, long count, vlong)
{
	int cn, qt;
	char buf[128];
	Buf *bp;
	Conn *c;
	Ether *e;
	Qid q;

	q = fid->qid;
	q.path &= ~fs->qid;
	e = fs->aux;
	qt = qtype(q.path);
	cn = qnum(q.path);
	switch(qt){
	case Qndata:
		c = getconn(e, cn, 0);
		if(c == nil){
			werrstr(Eio);
			return -1;
		}
		bp = allocbuf(e);
		if(count > sizeof(bp->data)-Hdrsize)
			count = sizeof(bp->data)-Hdrsize;
		memmove(bp->rp, data, count);
		bp->ndata = count;
		if(etherdebug > 1)
			dumpframe("etherout", bp->rp, bp->ndata);
		if(e->nblock == 0)
			sendp(e->wc, bp);
		else if(nbsendp(e->wc, bp) == 0){
			deprint(2, "%s: (out) packet lost\n", argv0);
			freebuf(e, bp);
		}
		break;
	case Qnctl:
		c = getconn(e, cn, 0);
		if(c == nil){
			werrstr(Eio);
			return -1;
		}
		if(count > sizeof(buf) - 1)
			count = sizeof(buf) - 1;
		memmove(buf, data, count);
		buf[count] = 0;
		if(etherctl(e, c, buf) < 0)
			return -1;
		break;
	default:
		sysfatal("usb: ether: fsread bug");
	}
	return count;
}

static int
openeps(Ether *e, int epin, int epout)
{
	e->epin = openep(e->dev, epin);
	if(e->epin == nil){
		fprint(2, "ether: in: openep %d: %r\n", epin);
		return -1;
	}
	if(epout == epin){
		incref(e->epin);
		e->epout = e->epin;
	}else
		e->epout = openep(e->dev, epout);
	if(e->epout == nil){
		fprint(2, "ether: out: openep %d: %r\n", epout);
		closedev(e->epin);
		return -1;
	}
	if(e->epin == e->epout)
		opendevdata(e->epin, ORDWR);
	else{
		opendevdata(e->epin, OREAD);
		opendevdata(e->epout, OWRITE);
	}
	if(e->epin->dfd < 0 || e->epout->dfd < 0){
		fprint(2, "ether: open i/o ep data: %r\n");
		closedev(e->epin);
		closedev(e->epout);
		return -1;
	}
	dprint(2, "ether: ep in %s maxpkt %d; ep out %s maxpkt %d\n",
		e->epin->dir, e->epin->maxpkt, e->epout->dir, e->epout->maxpkt);

	/* time outs are not activated for I/O endpoints */

	if(usbdebug > 2 || etherdebug > 2){
		devctl(e->epin, "debug 1");
		devctl(e->epout, "debug 1");
		devctl(e->dev, "debug 1");
	}

	return 0;
}

static int
usage(void)
{
	werrstr("usage: usb/ether [-d] [-N nb]");
	return -1;
}

static Usbfs etherfs = {
	.walk = fswalk,
	.open =	 fsopen,
	.read =	 fsread,
	.write = fswrite,
	.stat =	 fsstat,
	.clunk = fsclunk,
};

static void
shutdownchan(Channel *c)
{
	Buf *bp;

	while((bp=nbrecvp(c)) != nil)
		free(bp);
	chanfree(c);
}

static void
etherfree(Ether *e)
{
	int i;
	Buf *bp;

	if(e->free != nil)
		e->free(e);
	closedev(e->epin);
	closedev(e->epout);
	if(e->rc == nil){	/* not really started */
		free(e);
		return;
	}
	for(i = 0; i < e->nconns; i++)
		if(e->conns[i] != nil){
			while((bp = nbrecvp(e->conns[i]->rc)) != nil)
				free(bp);
			chanfree(e->conns[i]->rc);
			free(e->conns[i]);
		}
	shutdownchan(e->bc);
	shutdownchan(e->rc);
	shutdownchan(e->wc);
	e->epin = e->epout = nil;
	devctl(e->dev, "detach");
	free(e);
}

static void
etherdevfree(void *a)
{
	Ether *e = a;

	if(e != nil)
		etherfree(e);
}

/* must return 1 if c wants bp; 0 if not */
static int
cwantsbp(Conn *c, Buf *bp)
{
	if(c->ref != 0 && (c->prom != 0 || c->type < 0 || c->type == bp->type))
		return 1;
	return 0;
}

static void
etherwriteproc(void *a)
{
	Ether *e = a;
	Buf *bp;
	Channel *wc;

	wc = e->wc;
	while(e->exiting == 0){
		bp = recvp(wc);
		if(bp == nil || e->exiting != 0){
			free(bp);
			break;
		}
		e->nout++;
		if(e->bwrite(e, bp) < 0)
			e->noerrs++;
		if(isloopback(e, bp) && e->exiting == 0)
			sendp(e->rc, bp); /* send to input queue */
		else
			freebuf(e, bp);
	}
	deprint(2, "%s: writeproc exiting\n", argv0);
	closedev(e->dev);
}

static void
setbuftype(Buf *bp)
{
	uchar *p;

	bp->type = 0;
	if(bp->ndata >= Ehdrsize){
		p = bp->rp + Eaddrlen*2;
		bp->type = p[0]<<8 | p[1];
	}
}

static void
etherexiting(Ether *e)
{
	devctl(e->dev, "detach");
	e->exiting = 1;
	close(e->epin->dfd);
	e->epin->dfd = -1;
	close(e->epout->dfd);
	e->epout->dfd = -1;
	nbsend(e->wc, nil);
}

static void
etherreadproc(void *a)
{
	int i, n, nwants;
	Buf *bp, *dbp;
	Ether *e = a;

	while(e->exiting == 0){
		bp = nbrecvp(e->rc);
		if(bp == nil){
			bp = allocbuf(e);	/* leak() may think we leak */
			if(e->bread(e, bp) < 0){
				freebuf(e, bp);
				break;
			}
			if(bp->ndata == 0){
				/* may be a short packet; continue */
				if(0)dprint(2, "%s: read: short\n", argv0);
				freebuf(e, bp);
				continue;
			}else
				setbuftype(bp);
		}
		e->nin++;
		nwants = 0;
		for(i = 0; i < e->nconns; i++)
			nwants += cwantsbp(e->conns[i], bp);
		for(i = 0; nwants > 0 && i < e->nconns; i++)
			if(cwantsbp(e->conns[i], bp)){
				n = bp->ndata;
				if(e->conns[i]->type == -2 && n > 64)
					n = 64;
				if(nwants-- == 1){
					bp->ndata = n;
					dbp = bp;
					bp = nil;
				}else{
					dbp = allocbuf(e);
					memmove(dbp->rp, bp->rp, n);
					dbp->ndata = n;
					dbp->type = bp->type;
				}
				if(nbsendp(e->conns[i]->rc, dbp) == 0){
					e->nierrs++;
					freebuf(e, dbp);
				}
			}
		freebuf(e, bp);
	}
	deprint(2, "%s: writeproc exiting\n", argv0);
	etherexiting(e);
	closedev(e->dev);
	usbfsdel(&e->fs);
}

static void
setalt(Dev *d, int ifcid, int altid)
{
	if(usbcmd(d, Rh2d|Rstd|Riface, Rsetiface, altid, ifcid, nil, 0) < 0)
		dprint(2, "%s: setalt ifc %d alt %d: %r\n", argv0, ifcid, altid);
}

static int
ifaceinit(Ether *e, Iface *ifc, int *ei, int *eo)
{
	Ep *ep;
	int epin, epout, i;

	if(ifc == nil)
		return -1;

	epin = epout = -1;
	for(i = 0; (epin < 0 || epout < 0) && i < nelem(ifc->ep); i++)
		if((ep = ifc->ep[i]) != nil && ep->type == Ebulk){
			if(ep->dir == Eboth || ep->dir == Ein)
				if(epin == -1)
					epin =  ep->id;
			if(ep->dir == Eboth || ep->dir == Eout)
				if(epout == -1)
					epout = ep->id;
		}
	if(epin == -1 || epout == -1)
		return -1;

	dprint(2, "ether: ep ids: in %d out %d\n", epin, epout);
	for(i = 0; i < nelem(ifc->altc); i++)
		if(ifc->altc[i] != nil)
			setalt(e->dev, ifc->id, i);

	*ei = epin;
	*eo = epout;
	return 0;
}

static int
etherinit(Ether *e, int *ei, int *eo)
{
	int ctlid, datid, i, j;
	Conf *c;
	Desc *desc;
	Iface *ctlif, *datif;
	Usbdev *ud;

	*ei = *eo = -1;
	ud = e->dev->usb;

	/* look for union descriptor with ethernet ctrl interface */
	for(i = 0; i < nelem(ud->ddesc); i++){
		if((desc = ud->ddesc[i]) == nil)
			continue;
		if(desc->data.bLength < 5 || desc->data.bbytes[0] != Cdcunion)
			continue;

		ctlid = desc->data.bbytes[1];
		datid = desc->data.bbytes[2];

		if((c = desc->conf) == nil)
			continue;

		ctlif = datif = nil;
		for(j = 0; j < nelem(c->iface); j++){
			if(c->iface[j] == nil)
				continue;
			if(c->iface[j]->id == ctlid)
				ctlif = c->iface[j];
			if(c->iface[j]->id == datid)
				datif = c->iface[j];

			if(datif != nil && ctlif != nil){
				if(Subclass(ctlif->csp) == Scether &&
				    ifaceinit(e, datif, ei, eo) != -1)
					return 0;
				break;
			}
		}
	}
	/* try any other one that seems to be ok */
	for(i = 0; i < nelem(ud->conf); i++)
		if((c = ud->conf[i]) != nil)
			for(j = 0; j < nelem(c->iface); j++)
				if(ifaceinit(e, c->iface[j], ei, eo) != -1)
					return 0;
	dprint(2, "%s: no valid endpoints", argv0);
	return -1;
}

int
ethermain(Dev *dev, int argc, char **argv)
{
	int epin, epout, i, devid;
	Ether *e;

	devid = dev->id;
	ARGBEGIN{
	case 'd':
		if(etherdebug == 0)
			fprint(2, "ether debug on\n");
		etherdebug++;
		break;
	case 'N':
		devid = atoi(EARGF(usage()));
		break;
	default:
		return usage();
	}ARGEND
	if(argc != 0) {
		return usage();
	}
	e = dev->aux = emallocz(sizeof(Ether), 1);
	e->dev = dev;
	dev->free = etherdevfree;

	for(i = 0; i < nelem(ethers); i++)
		if(ethers[i](e) == 0)
			break;
	if(i == nelem(ethers))
		return -1;
	if(e->init == nil)
		e->init = etherinit;
	if(e->init(e, &epin, &epout) < 0)
		return -1;
	if(e->bwrite == nil)
		e->bwrite = etherbwrite;
	if(e->bread == nil)
		e->bread = etherbread;

	if(openeps(e, epin, epout) < 0)
		return -1;
	e->fs = etherfs;
	snprint(e->fs.name, sizeof(e->fs.name), "etherU%d", devid);
	e->fs.dev = dev;
	e->fs.aux = e;
	e->bc = chancreate(sizeof(Buf*), Nconns);
	e->rc = chancreate(sizeof(Buf*), Nconns/2);
	e->wc = chancreate(sizeof(Buf*), Nconns*2);
	incref(e->dev);
	proccreate(etherwriteproc, e, 16*1024);
	incref(e->dev);
	proccreate(etherreadproc, e, 16*1024);
	deprint(2, "%s: dev ref %ld\n", argv0, dev->ref);
	incref(e->dev);
	usbfsadd(&e->fs);
	return 0;
}
