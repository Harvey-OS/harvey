#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <9p.h>

#include "usb.h"

typedef struct Tab Tab;
typedef struct Qbuf Qbuf;
typedef struct Dq Dq;
typedef struct Conn Conn;
typedef struct Ehdr Ehdr;
typedef struct Stats Stats;

enum
{
	SC_ACM = 2,
	SC_ETHER = 6,

	FUNCTION = 0x24,
	FN_HEADER = 0,
	FN_UNION = 6,
	FN_ETHER = 15,
};

enum
{
	Qroot,
	Qiface,
	Qclone,
	Qstats,
	Qaddr,
	Qndir,
	Qctl,
	Qdata,
	Qtype,
	Qmax,
};

struct Tab
{
	char *name;
	ulong mode;
};

Tab tab[] =
{
	"/",		DMDIR|0555,
	"etherU",	DMDIR|0555,	/* calling it *ether* makes snoopy(8) happy */
	"clone",	0666,
	"stats",	0666,
	"addr",	0444,
	"%ud",	DMDIR|0555,
	"ctl",		0666,
	"data",	0666,
	"type",	0444,
};

struct Qbuf
{
	Qbuf		*next;
	int		ndata;
	uchar	data[];
};

struct Dq
{
	QLock	l;
	Dq		*next;
	Req		*r;
	Req		**rt;
	Qbuf		*q;
	Qbuf		**qt;

	int		nb;
};

struct Conn
{
	QLock	l;
	int		used;
	int		type;
	int		prom;
	Dq		*dq;
};

struct Ehdr
{
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];
};

struct Stats
{
	int		in;
	int		out;
};

Conn conn[32];
int nconn = 0;

int debug;
ulong time0;
int usbfdin = -1;
int usbfdout = -1;

Stats stats;

uchar macaddr[6];
int iunion[8][2];
int niunion;
int maxpacket = 64;

char *uname;

#define PATH(type, n)		((type)|((n)<<8))
#define TYPE(path)			(((uint)(path) & 0x000000FF)>>0)
#define NUM(path)			(((uint)(path) & 0xFFFFFF00)>>8)
#define NUMCONN(c)		(((long)(c)-(long)&conn[0])/sizeof(conn[0]))

static int
receivepacket(void *buf, int len);

static void
fillstat(Dir *d, uvlong path)
{
	Tab *t;

	memset(d, 0, sizeof(*d));
	d->uid = estrdup9p(uname);
	d->gid = estrdup9p(uname);
	d->qid.path = path;
	d->atime = d->mtime = time0;
	t = &tab[TYPE(path)];
	d->name = smprint(t->name, NUM(path));
	d->qid.type = t->mode>>24;
	d->mode = t->mode;
}

static void
fsattach(Req *r)
{
	if(r->ifcall.aname && r->ifcall.aname[0]){
		respond(r, "invalid attach specifier");
		return;
	}
	if(uname == nil)
		uname = estrdup9p(r->ifcall.uname);
	r->fid->qid.path = PATH(Qroot, 0);
	r->fid->qid.type = QTDIR;
	r->fid->qid.vers = 0;
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

static void
fsstat(Req *r)
{
	fillstat(&r->d, r->fid->qid.path);
	respond(r, nil);
}

static int
rootgen(int i, Dir *d, void*)
{
	i += Qroot+1;
	if(i == Qiface){
		fillstat(d, i);
		return 0;
	}
	return -1;
}

static int
ifacegen(int i, Dir *d, void*)
{
	i += Qiface+1;
	if(i < Qndir){
		fillstat(d, i);
		return 0;
	}
	i -= Qndir;
	if(i < nconn){
		fillstat(d, PATH(Qndir, i));
		return 0;
	}
	return -1;
}

static int
ndirgen(int i, Dir *d, void *aux)
{
	i += Qndir+1;
	if(i < Qmax){
		fillstat(d, PATH(i, NUMCONN(aux)));
		return 0;
	}
	return -1;
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i, n;
	char buf[32];
	ulong path;

	path = fid->qid.path;
	if(!(fid->qid.type&QTDIR))
		return "walk in non-directory";

	if(strcmp(name, "..") == 0){
		switch(TYPE(path)){
		case Qroot:
			return nil;
		case Qiface:
			qid->path = PATH(Qroot, 0);
			qid->type = tab[Qroot].mode>>24;
			return nil;
		case Qndir:
			qid->path = PATH(Qiface, 0);
			qid->type = tab[Qiface].mode>>24;
			return nil;
		default:
			return "bug in fswalk1";
		}
	}

	for(i = TYPE(path)+1; i<nelem(tab); i++){
		if(i==Qndir){
			n = atoi(name);
			snprint(buf, sizeof buf, "%d", n);
			if(n < nconn && strcmp(buf, name) == 0){
				qid->path = PATH(i, n);
				qid->type = tab[i].mode>>24;
				return nil;
			}
			break;
		}
		if(strcmp(name, tab[i].name) == 0){
			qid->path = PATH(i, NUM(path));
			qid->type = tab[i].mode>>24;
			return nil;
		}
		if(tab[i].mode&DMDIR)
			break;
	}
	return "directory entry not found";
}

static void
matchrq(Dq *d)
{
	Req *r;
	Qbuf *b;

	while(r = d->r){
		int n;

		if((b = d->q) == nil)
			break;
		if((d->q = b->next) == nil)
			d->qt = &d->q;
		if((d->r = (Req*)r->aux) == nil)
			d->rt = &d->r;

		n = r->ifcall.count;
		if(n > b->ndata)
			n = b->ndata;
		memmove(r->ofcall.data, b->data, n);
		free(b);
		r->ofcall.count = n;
		respond(r, nil);
	}
}

static void
readconndata(Req *r)
{
	Dq *d;

	d = r->fid->aux;
	qlock(&d->l);
	if(d->q==nil && d->nb){
		qunlock(&d->l);
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}
	// enqueue request
	r->aux = nil;
	*d->rt = r;
	d->rt = (Req**)&r->aux;
	matchrq(d);
	qunlock(&d->l);
}

static void
writeconndata(Req *r)
{
	char e[ERRMAX];
	Dq *d;
	void *p;
	int n;

	d = r->fid->aux;
	p = r->ifcall.data;
	n = r->ifcall.count;

	if((n == 11) && memcmp(p, "nonblocking", n)==0){
		d->nb = 1;
		goto out;
	}

	n = write(usbfdout, p, n);
	if(n < 0){
		rerrstr(e, sizeof(e));
		respond(r, e);
		return;
	}
	/*
	 * this may not work with all CDC devices. the
	 * linux driver sends one more random byte
	 * instead of a zero byte transaction. maybe we
	 * should do the same?
	 */
	if(n % maxpacket == 0)
		write(usbfdout, "", 0);

	if(receivepacket(p, n) == 0)
		stats.out++;
out:
	r->ofcall.count = n;
	respond(r, nil);
}

static char*
mac2str(uchar *m)
{
	int i;
	char *t = "0123456789abcdef";
	static char buf[13];
	buf[13] = 0;
	for(i=0; i<6; i++){
		buf[i*2] = t[m[i]>>4];
		buf[i*2+1] = t[m[i]&0xF];
	}
	return buf;
}

static int
str2mac(uchar *m, char *s)
{
	int i;

	if(strlen(s) != 12)
		return -1;

	for(i=0; i<12; i++){
		uchar v;

		if(s[i] >= 'A' && s[i] <= 'F'){
			v = 10 + s[i] - 'A';
		} else if(s[i] >= 'a' && s[i] <= 'f'){
			v = 10 + s[i] - 'a';
		} else if(s[i] >= '0' && s[i] <= '9'){
			v = s[i] - '0';
		} else {
			v = 0;
		}
		if(i&1){
			m[i/2] |= v;
		} else {
			m[i/2] = v<<4;
		}
	}
	return 0;
}

static void
fsread(Req *r)
{
	char buf[200];
	char e[ERRMAX];
	ulong path;

	path = r->fid->qid.path;

	switch(TYPE(path)){
	default:
		snprint(e, sizeof e, "bug in fsread path=%lux", path);
		respond(r, e);
		break;

	case Qroot:
		dirread9p(r, rootgen, nil);
		respond(r, nil);
		break;

	case Qiface:
		dirread9p(r, ifacegen, nil);
		respond(r, nil);
		break;

	case Qstats:
		snprint(buf, sizeof(buf),
			"in: %d\n"
			"out: %d\n"
			"mbps: %d\n"
			"addr: %s\n",
			stats.in, stats.out, 10, mac2str(macaddr));
		readstr(r, buf);
		respond(r, nil);
		break;

	case Qaddr:
		readstr(r, mac2str(macaddr));
		respond(r, nil);
		break;

	case Qndir:
		dirread9p(r, ndirgen, &conn[NUM(path)]);
		respond(r, nil);
		break;

	case Qctl:
		snprint(buf, sizeof(buf), "%11d ", NUM(path));
		readstr(r, buf);
		respond(r, nil);
		break;

	case Qtype:
		snprint(buf, sizeof(buf), "%11d ", conn[NUM(path)].type);
		readstr(r, buf);
		respond(r, nil);
		break;

	case Qdata:
		readconndata(r);
		break;
	}
}

static void
fswrite(Req *r)
{
	char e[ERRMAX];
	ulong path;
	char *p;
	int n;

	path = r->fid->qid.path;
	switch(TYPE(path)){
	case Qctl:
		n = r->ifcall.count;
		p = (char*)r->ifcall.data;
		if((n == 11) && memcmp(p, "promiscuous", 11)==0)
			conn[NUM(path)].prom = 1;
		if((n > 8) && memcmp(p, "connect ", 8)==0){
			char x[12];

			if(n - 8 >= sizeof(x)){
				respond(r, "invalid control msg");
				return;
			}

			p += 8;
			memcpy(x, p, n-8);
			x[n-8] = 0;

			conn[NUM(path)].type = atoi(p);
		}
		r->ofcall.count = n;
		respond(r, nil);
		break;
	case Qdata:
		writeconndata(r);
		break;
	default:
		snprint(e, sizeof e, "bug in fswrite path=%lux", path);
		respond(r, e);
	}
}

static void
fsopen(Req *r)
{
	static int need[4] = { 4, 2, 6, 1 };
	ulong path;
	int i, n;
	Tab *t;
	Dq *d;
	Conn *c;


	/*
	 * lib9p already handles the blatantly obvious.
	 * we just have to enforce the permissions we have set.
	 */
	path = r->fid->qid.path;
	t = &tab[TYPE(path)];
	n = need[r->ifcall.mode&3];
	if((n&t->mode) != n){
		respond(r, "permission denied");
		return;
	}

	d = nil;
	r->fid->aux = nil;

	switch(TYPE(path)){
	case Qclone:
		for(i=0; i<nelem(conn); i++){
			if(conn[i].used)
				continue;
			if(i >= nconn)
				nconn = i+1;
			path = PATH(Qctl, i);
			goto CaseConn;
		}
		respond(r, "out of connections");
		return;
	case Qdata:
		d = emalloc9p(sizeof(*d));
		memset(d, 0, sizeof(*d));
		d->qt = &d->q;
		d->rt = &d->r;
		r->fid->aux = d;
	case Qndir:
	case Qctl:
	case Qtype:
	CaseConn:
		c = &conn[NUM(path)];
		qlock(&c->l);
		if(c->used++ == 0){
			c->type = 0;
			c->prom = 0;
		}
		if(d != nil){
			d->next = c->dq;
			c->dq = d;
		}
		qunlock(&c->l);
		break;
	}

	r->fid->qid.path = path;
	r->ofcall.qid.path = path;
	respond(r, nil);
}

static void
fsflush(Req *r)
{
	Req *o, **p;
	Fid *f;
	Dq *d;

	o = r->oldreq;
	f = o->fid;
	if(TYPE(f->qid.path) == Qdata){
		d = f->aux;
		qlock(&d->l);
		for(p=&d->r; *p; p=(Req**)&((*p)->aux)){
			if(*p == o){
				if((*p = (Req*)o->aux) == nil)
					d->rt = p;
				r->oldreq = nil;
				respond(o, "interrupted");
				break;
			}
		}
		qunlock(&d->l);
	}
	respond(r, nil);
}


static void
fsdestroyfid(Fid *fid)
{
	Conn *c;
	Qbuf *b;
	Dq **x, *d;

	if(TYPE(fid->qid.path) >= Qndir){
		c = &conn[NUM(fid->qid.path)];
		qlock(&c->l);
		if(d = fid->aux){
			fid->aux = nil;
			for(x=&c->dq; *x; x=&((*x)->next)){
				if(*x == d){
					*x = d->next;
					break;
				}
			}
			qlock(&d->l);
			while(b = d->q){
				d->q = b->next;
				free(b);
			}
			qunlock(&d->l);
		}
		if(TYPE(fid->qid.path) == Qctl)
			c->prom = 0;
		c->used--;
		qunlock(&c->l);
	}
}

static char*
finddevice(int *ctrlno, int *id)
{
	static char buf[80];
	int fd;
	int i, j;

	for(i=0; i<16; i++){
		for(j=0; j<128; j++){
			int csp;
			char line[80];
			char *p;
			int n;

			snprint(buf, sizeof(buf), "/dev/usb%d/%d/status", i, j);
			fd = open(buf, OREAD);
			buf[strlen(buf)-7] = 0;

			if(fd < 0)
				break;
			n = read(fd, line, sizeof(line)-1);
			close(fd);
			if(n <= 0)
				continue;
			line[n] = 0;
			p = line;

			if(strncmp(p, "Enabled ", 8) == 0)
				p += 8;

			csp = atol(p);

			if(Class(csp) != Clcomms)
				continue;
			switch(Subclass(csp)){
			default:
				continue;
			case SC_ACM:
			case SC_ETHER:
				break;
			}
			if(debug)
				fprint(2, "found matching device %s\n", buf);
			if(ctrlno)
				*ctrlno = i;
			if(id)
				*id = j;
			return buf;
		}
	}
	return nil;
}

static char*
usbgetstr(Dev *d, int i, int lang)
{
	byte b[200];
	byte *rb;
	char *s;
	Rune r;
	int l;
	int n;

	setupreq(d->ep[0], RD2H|Rdevice, GET_DESCRIPTOR, STRING<<8|i, lang, sizeof(b));
	if((n = setupreply(d->ep[0],  b, sizeof(b))) < 0)
		return nil;
	if(n <= 2)
		return nil;
	if(n & 1)
		return nil;
	s = malloc(n*UTFmax+1);
	n = (n - 2)/2;
	rb = (byte*)b + 2;
	for(l=0; --n >= 0; rb += 2){
		r = GET2(rb);
		l += runetochar(s+l, &r);
	}
	s[l] = 0;
	return s;
}

static void
etherfunc(Device *d, int, ulong csp, void *bb, int nb)
{
	int class, subclass;
	uchar *b = bb;
	char *s;

	class = Class(csp);
	subclass = Subclass(csp);

	if(class != CL_COMMS || subclass != SC_ETHER)
		return;
	switch(b[2]){
	case FN_HEADER:
		pcs_raw("header: ", bb, nb);
		break;
	case FN_ETHER:
		pcs_raw("ether: ", bb, nb);
		if(s = usbgetstr(d, b[3], 0)){
			str2mac(macaddr, s);
			free(s);
		}
		break;
	case FN_UNION:
		pcs_raw("union: ", bb, nb);
		if(niunion < nelem(iunion)){
			iunion[niunion][0] = b[3];
			iunion[niunion][1] = b[4];
			niunion++;
		}
		break;
	default:
		pcs_raw("unknown: ", bb, nb);
	}
}

int
findendpoints(Device *d, int *epin, int *epout)
{
	int i, j, k;

	*epin = *epout = -1;
	niunion = 0;
	memset(macaddr, 0, 6);

	for(i=0; i<d->nconf; i++){
		if (d->config[i] == nil)
			d->config[i] = mallocz(sizeof(*d->config[0]), 1);
		loadconfig(d, i);
	}
	if(niunion <= 0)
		return -1;
	for(i=0; i<nelem(d->ep); i++){
		Endpt *ep;

		if((ep = d->ep[i]) == nil)
			continue;
		if(ep->type != Ebulk)
			continue;
		if(ep->iface == nil)
			continue;
		if(Class(ep->iface->csp) != CL_DATA)
			continue;
		for(j=0; j<niunion; j++){
			if(iunion[j][1] != ep->iface->interface)
				continue;
			if(ep->conf == nil)
				continue;
			for(k=0; k<ep->conf->nif; k++){
				if(iunion[j][0] != ep->conf->iface[k]->interface)
					continue;
				if(Class(ep->conf->iface[k]->csp) != CL_COMMS)
					continue;
				if(Subclass(ep->conf->iface[k]->csp) != SC_ETHER)
					continue;
				if(ep->addr & 0x80){
					if(*epin == -1)
						*epin = ep->addr&0xF;
				} else {
					if(*epout == -1)
						*epout = ep->addr&0xF;
				}
				if (*epin != -1 && *epout != -1){
					maxpacket = ep->maxpkt;
					for(i=0; i<2; i++){
						if(ep->iface->dalt[i] == nil)
							continue;
						setupreq(d->ep[0], RH2D|Rinterface, SET_INTERFACE, i, ep->iface->interface, 0);
						break;
					}
					return 0;
				}
				break;
			}
			break;
		}
	}
	return -1;
}


static int
inote(void *, char *msg)
{
	if(strstr(msg, "interrupt"))
		return 1;
	return 0;
}

static int
receivepacket(void *buf, int len)
{
	int i;
	int t;
	Ehdr *h;

	if(len < sizeof(*h))
		return -1;

	h = (Ehdr*)buf;
	t = (h->type[0]<<8)|h->type[1];

	for(i=0; i<nconn; i++){
		Qbuf *b;
		Conn *c;
		Dq *d;

		c = &conn[i];
		qlock(&c->l);
		if(!c->used)
			goto next;
		if(c->type > 0)
			if(c->type != t)
				goto next;
		if(!c->prom && !(h->d[0]&1))
			if(memcmp(h->d, macaddr, 6))
				goto next;
		for(d=c->dq; d; d=d->next){
			int n;

			n = len;
			if(c->type == -2 && n > 64)
				n = 64;

			b = emalloc9p(sizeof(*b) + n);
			b->ndata = n;
			memcpy(b->data, buf, n);

			qlock(&d->l);
			// enqueue buffer
			b->next = nil;
			*d->qt = b;
			d->qt = &b->next;
			matchrq(d);
			qunlock(&d->l);
		}
next:
		qunlock(&c->l);
	}
	return 0;
}

static void
usbreadproc(void *)
{
	char err[ERRMAX];
	uchar buf[4*1024];
	atnotify(inote, 1);

	threadsetname("usbreadproc");

	for(;;){
		int n;

		n = read(usbfdin, buf, sizeof(buf));
		if(n < 0){
			rerrstr(err, sizeof(err));
			if(strstr(err, "interrupted"))
				continue;
			fprint(2, "usbreadproc: %s\n", err);
			threadexitsall(err);
		}
		if(n == 0)
			continue;
		if(receivepacket(buf, n) == 0)
			stats.in++;
	}
}

void (*dprinter[])(Device *, int, ulong, void *b, int n) = {
	[STRING] pstring,
	[DEVICE] pdevice,
	[FUNCTION] etherfunc,
};

Srv fs =
{
.attach=		fsattach,
.destroyfid=	fsdestroyfid,
.walk1=		fswalk1,
.open=		fsopen,
.read=		fsread,
.write=		fswrite,
.stat=		fsstat,
.flush=		fsflush,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-dD] [-m mtpt] [-s srv] [ctrlno n]\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *srv, *mtpt, *r;
	char s[64];
	int ctrlno, id, epin, epout;
	Device *d;

	srv = nil;
	mtpt = "/net";

	ARGBEGIN {
	case 'd':
		debug = 1;
		break;
	case 'D':
		chatty9p++;
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 's':
		srv = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND;

	if(argc != 0 && argc != 2)
		usage();

	if(argc == 2){
		ctrlno = atoi(argv[0]);
		id = atoi(argv[1]);
		r = smprint("/dev/usb%d/%d", ctrlno, id);
	} else {
		r = finddevice(&ctrlno, &id);
		if(r == nil){
			fprint(2, "no device found\n");
			return;
		}
	}

	if(debug)
		fprint(2, "using %d %d %s\n", ctrlno, id, r);

	if((d = opendev(ctrlno, id)) == nil){
		fprint(2, "opendev failed: %r\n");
		exits("opendev");
	}
	if(describedevice(d) < 0){
		fprint(2, "describedevice failed: %r\n");
		exits("describedevice");
	}
	if(findendpoints(d, &epin, &epout)  < 0){
		fprint(2, "no endpoints found!\n");
		exits("findendpoints");
	}

	if(debug)
		fprint(2, "endpoints in %d, out %d, maxpacket %d\n", epin, epout, maxpacket);

	fprint(d->ctl, "ep %d bulk r %d 24", epin, maxpacket);
	fprint(d->ctl, "ep %d bulk w %d 24", epout, maxpacket);
	closedev(d);

	sprint(s, "%s/ep%ddata", r, epin);
	if((usbfdin = open(s, OREAD)) < 0){
		fprint(2, "cant open receiving endpoint: %r\n");
		exits("open");
	}

	sprint(s, "%s/ep%ddata", r, epout);
	if((usbfdout = open(s, OWRITE)) < 0){
		fprint(2, "cant open transmitting endpoint: %r\n");
		exits("open");
	}

	proccreate(usbreadproc, nil, 8*1024);

	atnotify(inote, 1);
	time0 = time(0);
	threadpostmountsrv(&fs, srv, mtpt, MBEFORE);
}
