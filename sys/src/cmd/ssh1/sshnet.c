/*
 * SSH network file system.
 * Presents remote TCP stack as /net-style file system.
 */

#include "ssh.h"
#include <bio.h>
#include <ndb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>

int rawhack = 1;
Conn *conn;
char *remoteip	= "<remote>";
char *mtpt;

Cipher *allcipher[] = {
	&cipherrc4,
	&cipherblowfish,
	&cipher3des,
	&cipherdes,
	&ciphernone,
	&ciphertwiddle,
};

Auth *allauth[] = {
	&authpassword,
	&authrsa,
	&authtis,
};

char *cipherlist = "rc4 3des";
char *authlist = "rsa password tis";

Cipher*
findcipher(char *name, Cipher **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown cipher %s", name);
	return nil;
}

Auth*
findauth(char *name, Auth **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown auth %s", name);
	return nil;
}

void
usage(void)
{
	fprint(2, "usage: sshnet [-A authlist] [-c cipherlist] [-m mtpt] [user@]hostname\n");
	exits("usage");
}

int
isatty(int fd)
{
	char buf[64];

	buf[0] = '\0';
	fd2path(fd, buf, sizeof buf);
	if(strlen(buf)>=9 && strcmp(buf+strlen(buf)-9, "/dev/cons")==0)
		return 1;
	return 0;
}

enum
{
	Qroot,
	Qcs,
	Qtcp,
	Qclone,
	Qn,
	Qctl,
	Qdata,
	Qlocal,
	Qremote,
	Qstatus,
};

#define PATH(type, n)		((type)|((n)<<8))
#define TYPE(path)			((int)(path) & 0xFF)
#define NUM(path)			((uint)(path)>>8)

Channel *sshmsgchan;		/* chan(Msg*) */
Channel *fsreqchan;			/* chan(Req*) */
Channel *fsreqwaitchan;		/* chan(nil) */
Channel *fsclunkchan;		/* chan(Fid*) */
Channel *fsclunkwaitchan;	/* chan(nil) */
ulong time0;

enum
{
	Closed,
	Dialing,
	Established,
	Teardown,
};

char *statestr[] = {
	"Closed",
	"Dialing",
	"Established",
	"Teardown",
};

typedef struct Client Client;
struct Client
{
	int ref;
	int state;
	int num;
	int servernum;
	char *connect;
	Req *rq;
	Req **erq;
	Msg *mq;
	Msg **emq;
};

int nclient;
Client **client;

int
newclient(void)
{
	int i;
	Client *c;

	for(i=0; i<nclient; i++)
		if(client[i]->ref==0 && client[i]->state == Closed)
			return i;

	if(nclient%16 == 0)
		client = erealloc9p(client, (nclient+16)*sizeof(client[0]));

	c = emalloc9p(sizeof(Client));
	memset(c, 0, sizeof(*c));
	c->num = nclient;
	client[nclient++] = c;
	return c->num;
}

void
queuereq(Client *c, Req *r)
{
	if(c->rq==nil)
		c->erq = &c->rq;
	*c->erq = r;
	r->aux = nil;
	c->erq = (Req**)&r->aux;
}

void
queuemsg(Client *c, Msg *m)
{
	if(c->mq==nil)
		c->emq = &c->mq;
	*c->emq = m;
	m->link = nil;
	c->emq = (Msg**)&m->link;
}

void
matchmsgs(Client *c)
{
	Req *r;
	Msg *m;
	int n, rm;

	while(c->rq && c->mq){
		r = c->rq;
		c->rq = r->aux;

		rm = 0;
		m = c->mq;
		n = r->ifcall.count;
		if(n >= m->ep - m->rp){
			n = m->ep - m->rp;
			c->mq = m->link;
			rm = 1;
		}
		memmove(r->ofcall.data, m->rp, n);
		if(rm)
			free(m);
		else
			m->rp += n;
		r->ofcall.count = n;
		respond(r, nil);
	}
}

Req*
findreq(Client *c, Req *r)
{
	Req **l;

	for(l=&c->rq; *l; l=(Req**)&(*l)->aux){
		if(*l == r){
			*l = r->aux;
			if(*l == nil)
				c->erq = l;
			return r;
		}
	}
	return nil;
}

void
dialedclient(Client *c)
{
	Req *r;

	if(r=c->rq){
		if(r->aux != nil)
			sysfatal("more than one outstanding dial request (BUG)");
		if(c->state == Established)
			respond(r, nil);
		else
			respond(r, "connect failed");
	}
	c->rq = nil;
}

void
teardownclient(Client *c)
{
	Msg *m;

	c->state = Teardown;
	m = allocmsg(conn, SSH_MSG_CHANNEL_INPUT_EOF, 4);
	putlong(m, c->servernum);
	sendmsg(m);
}

void
hangupclient(Client *c)
{
	Req *r, *next;
	Msg *m, *mnext;

	c->state = Closed;
	for(m=c->mq; m; m=mnext){
		mnext = m->link;
		free(m);
	}
	c->mq = nil;
	for(r=c->rq; r; r=next){
		next = r->aux;
		respond(r, "hangup on network connection");
	}
	c->rq = nil;
}

void
closeclient(Client *c)
{
	Msg *m, *next;

	if(--c->ref)
		return;

	if(c->rq != nil)
		sysfatal("ref count reached zero with requests pending (BUG)");

	for(m=c->mq; m; m=next){
		next = m->link;
		free(m);
	}
	c->mq = nil;

	if(c->state != Closed)
		teardownclient(c);
}

	
void
sshreadproc(void *a)
{
	Conn *c;
	Msg *m;

	c = a;
	for(;;){
		m = recvmsg(c, -1);
		if(m == nil)
			sysfatal("eof on ssh connection");
		sendp(sshmsgchan, m);
	}
}

typedef struct Tab Tab;
struct Tab
{
	char *name;
	ulong mode;
};

Tab tab[] =
{
	"/",		DMDIR|0555,
	"cs",		0666,
	"tcp",	DMDIR|0555,	
	"clone",	0666,
	nil,		DMDIR|0555,
	"ctl",		0666,
	"data",	0666,
	"local",	0444,
	"remote",	0444,
	"status",	0444,
};

static void
fillstat(Dir *d, uvlong path)
{
	Tab *t;

	memset(d, 0, sizeof(*d));
	d->uid = estrdup9p("ssh");
	d->gid = estrdup9p("ssh");
	d->qid.path = path;
	d->atime = d->mtime = time0;
	t = &tab[TYPE(path)];
	if(t->name)
		d->name = estrdup9p(t->name);
	else{
		d->name = smprint("%ud", NUM(path));
		if(d->name == nil)
			sysfatal("out of memory");
	}
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
	if(i <= Qtcp){
		fillstat(d, i);
		return 0;
	}
	return -1;
}

static int
tcpgen(int i, Dir *d, void*)
{
	i += Qtcp+1;
	if(i < Qn){
		fillstat(d, i);
		return 0;
	}
	i -= Qn;
	if(i < nclient){
		fillstat(d, PATH(Qn, i));
		return 0;
	}
	return -1;
}

static int
clientgen(int i, Dir *d, void *aux)
{
	Client *c;

	c = aux;
	i += Qn+1;
	if(i <= Qstatus){
		fillstat(d, PATH(i, c->num));
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
		case Qn:
			qid->path = PATH(Qtcp, NUM(path));
			qid->type = tab[Qtcp].mode>>24;
			return nil;
		case Qtcp:
			qid->path = PATH(Qroot, 0);
			qid->type = tab[Qroot].mode>>24;
			return nil;
		case Qroot:
			return nil;
		default:
			return "bug in fswalk1";
		}
	}

	i = TYPE(path)+1;
	for(; i<nelem(tab); i++){
		if(i==Qn){
			n = atoi(name);
			snprint(buf, sizeof buf, "%d", n);
			if(n < nclient && strcmp(buf, name) == 0){
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

typedef struct Cs Cs;
struct Cs
{
	char *resp;
	int isnew;
};

static int
ndbfindport(char *p)
{
	char *s, *port;
	int n;
	static Ndb *db;

	if(*p == '\0')
		return -1;

	n = strtol(p, &s, 0);
	if(*s == '\0')
		return n;

	if(db == nil){
		db = ndbopen("/lib/ndb/common");
		if(db == nil)
			return -1;
	}

	port = ndbgetvalue(db, nil, "tcp", p, "port", nil);
	if(port == nil)
		return -1;
	n = atoi(port);
	free(port);

	return n;
}	

static void
csread(Req *r)
{
	Cs *cs;

	cs = r->fid->aux;
	if(cs->resp==nil){
		respond(r, "cs read without write");
		return;
	}
	if(r->ifcall.offset==0){
		if(!cs->isnew){
			r->ofcall.count = 0;
			respond(r, nil);
			return;
		}
		cs->isnew = 0;
	}
	readstr(r, cs->resp);
	respond(r, nil);
}

static void
cswrite(Req *r)
{
	int port, nf;
	char err[ERRMAX], *f[4], *s, *ns;
	Cs *cs;

	cs = r->fid->aux;
	s = emalloc(r->ifcall.count+1);
	memmove(s, r->ifcall.data, r->ifcall.count);
	s[r->ifcall.count] = '\0';

	nf = getfields(s, f, nelem(f), 0, "!");
	if(nf != 3){
		free(s);
		respond(r, "can't translate");
		return;
	}
	if(strcmp(f[0], "tcp") != 0 && strcmp(f[0], "net") != 0){
		free(s);
		respond(r, "unknown protocol");
		return;
	}
	port = ndbfindport(f[2]);
	if(port <= 0){
		free(s);
		respond(r, "no translation found");
		return;
	}

	ns = smprint("%s/tcp/clone %s!%d", mtpt, f[1], port);
	if(ns == nil){
		free(s);
		rerrstr(err, sizeof err);
		respond(r, err);
		return;
	}
	free(s);
	free(cs->resp);
	cs->resp = ns;
	cs->isnew = 1;
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

static void
ctlread(Req *r, Client *c)
{
	char buf[32];

	sprint(buf, "%d", c->num);
	readstr(r, buf);
	respond(r, nil);
}

static void
ctlwrite(Req *r, Client *c)
{
	char *f[3], *s;
	int nf;
	Msg *m;

	s = emalloc(r->ifcall.count+1);
	memmove(s, r->ifcall.data, r->ifcall.count);
	s[r->ifcall.count] = '\0';

	nf = tokenize(s, f, 3);
	if(nf == 0){
		free(s);
		respond(r, nil);
		return;
	}

	if(strcmp(f[0], "hangup") == 0){
		if(c->state != Established)
			goto Badarg;
		if(nf != 1)
			goto Badarg;
		queuereq(c, r);
		teardownclient(c);
	}else if(strcmp(f[0], "connect") == 0){
		if(c->state != Closed)
			goto Badarg;
		if(nf != 2)
			goto Badarg;
		c->connect = estrdup9p(f[1]);
		nf = getfields(f[1], f, nelem(f), 0, "!");
		if(nf != 2){
			free(c->connect);
			c->connect = nil;
			goto Badarg;
		}
		c->state = Dialing;
		m = allocmsg(conn, SSH_MSG_PORT_OPEN, 4+4+strlen(f[0])+4+4+strlen("localhost"));
		putlong(m, c->num);
		putstring(m, f[0]);
		putlong(m, ndbfindport(f[1]));
		putstring(m, "localhost");
		queuereq(c, r);
		sendmsg(m);
	}else{
	Badarg:
		respond(r, "bad or inappropriate tcp control message");
	}
	free(s);
}

static void
dataread(Req *r, Client *c)
{
	if(c->state != Established){
		respond(r, "not connected");
		return;
	}
	queuereq(c, r);
	matchmsgs(c);
}

static void
datawrite(Req *r, Client *c)
{
	Msg *m;

	if(c->state != Established){
		respond(r, "not connected");
		return;
	}
	if(r->ifcall.count){
		m = allocmsg(conn, SSH_MSG_CHANNEL_DATA, 4+4+r->ifcall.count);
		putlong(m, c->servernum);
		putlong(m, r->ifcall.count);
		putbytes(m, r->ifcall.data, r->ifcall.count);
		sendmsg(m);
	}
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

static void
localread(Req *r)
{
	char buf[128];

	snprint(buf, sizeof buf, "%s!%d\n", remoteip, 0);
	readstr(r, buf);
	respond(r, nil);
}

static void
remoteread(Req *r, Client *c)
{
	char *s;
	char buf[128];

	s = c->connect;
	if(s == nil)
		s = "::!0";
	snprint(buf, sizeof buf, "%s\n", s);
	readstr(r, buf);
	respond(r, nil);
}

static void
statusread(Req *r, Client *c)
{
	char buf[64];
	char *s;

	snprint(buf, sizeof buf, "%s!%d", remoteip, 0);
	s = statestr[c->state];
	readstr(r, s);
	respond(r, nil);
}

static void
fsread(Req *r)
{
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

	case Qcs:
		csread(r);
		break;

	case Qtcp:
		dirread9p(r, tcpgen, nil);
		respond(r, nil);
		break;

	case Qn:
		dirread9p(r, clientgen, client[NUM(path)]);
		respond(r, nil);
		break;

	case Qctl:
		ctlread(r, client[NUM(path)]);
		break;

	case Qdata:
		dataread(r, client[NUM(path)]);
		break;

	case Qlocal:
		localread(r);
		break;

	case Qremote:
		remoteread(r, client[NUM(path)]);
		break;

	case Qstatus:
		statusread(r, client[NUM(path)]);
		break;
	}
}

static void
fswrite(Req *r)
{
	ulong path;
	char e[ERRMAX];

	path = r->fid->qid.path;
	switch(TYPE(path)){
	default:
		snprint(e, sizeof e, "bug in fswrite path=%lux", path);
		respond(r, e);
		break;

	case Qcs:
		cswrite(r);
		break;

	case Qctl:
		ctlwrite(r, client[NUM(path)]);
		break;

	case Qdata:
		datawrite(r, client[NUM(path)]);
		break;
	}
}

static void
fsopen(Req *r)
{
	static int need[4] = { 4, 2, 6, 1 };
	ulong path;
	int n;
	Tab *t;
	Cs *cs;

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

	switch(TYPE(path)){
	case Qcs:
		cs = emalloc(sizeof(Cs));
		r->fid->aux = cs;
		respond(r, nil);
		break;
	case Qclone:
		n = newclient();
		path = PATH(Qctl, n);
		r->fid->qid.path = path;
		r->ofcall.qid.path = path;
		if(chatty9p)
			fprint(2, "open clone => path=%lux\n", path);
		t = &tab[Qctl];
		/* fall through */
	default:
		if(t-tab >= Qn)
			client[NUM(path)]->ref++;
		respond(r, nil);
		break;
	}
}

static void
fsflush(Req *r)
{
	int i;

	for(i=0; i<nclient; i++)
		if(findreq(client[i], r->oldreq))
			respond(r->oldreq, "interrupted");
	respond(r, nil);
}

static void
handlemsg(Msg *m)
{
	int chan, n;
	Client *c;

	switch(m->type){
	case SSH_MSG_DISCONNECT:
	case SSH_CMSG_EXIT_CONFIRMATION:
		sysfatal("disconnect");

	case SSH_CMSG_STDIN_DATA:
	case SSH_CMSG_EOF:
	case SSH_CMSG_WINDOW_SIZE:
		/* don't care */
		free(m);
		break;

	case SSH_MSG_CHANNEL_DATA:
		chan = getlong(m);
		n = getlong(m);
		if(m->rp+n != m->ep)
			sysfatal("got bad channel data");
		if(chan<nclient && (c=client[chan])->state==Established){
			queuemsg(c, m);
			matchmsgs(c);
		}else
			free(m);
		break;

	case SSH_MSG_CHANNEL_INPUT_EOF:
		chan = getlong(m);
		free(m);
		if(chan<nclient){
			c = client[chan];
			chan = c->servernum;
			hangupclient(c);
			m = allocmsg(conn, SSH_MSG_CHANNEL_OUTPUT_CLOSED, 4);
			putlong(m, chan);
			sendmsg(m);
		}
		break;

	case SSH_MSG_CHANNEL_OUTPUT_CLOSED:
		chan = getlong(m);
		if(chan<nclient)
			hangupclient(client[chan]);
		free(m);
		break;

	case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		chan = getlong(m);
		c = nil;
		if(chan>=nclient || (c=client[chan])->state != Dialing){
			if(c)
				fprint(2, "cstate %d\n", c->state);
			sysfatal("got unexpected open confirmation for %d", chan);
		}
		c->servernum = getlong(m);
		c->state = Established;
		dialedclient(c);
		free(m);
		break;

	case SSH_MSG_CHANNEL_OPEN_FAILURE:
		chan = getlong(m);
		c = nil;
		if(chan>=nclient || (c=client[chan])->state != Dialing)
			sysfatal("got unexpected open failure");
		if(m->rp+4 <= m->ep)
			c->servernum = getlong(m);
		c->state = Closed;
		dialedclient(c);
		free(m);
		break;
	}
}

void
fsnetproc(void*)
{
	ulong path;
	Alt a[4];
	Cs *cs;
	Fid *fid;
	Req *r;
	Msg *m;

	threadsetname("fsthread");

	a[0].op = CHANRCV;
	a[0].c = fsclunkchan;
	a[0].v = &fid;
	a[1].op = CHANRCV;
	a[1].c = fsreqchan;
	a[1].v = &r;
	a[2].op = CHANRCV;
	a[2].c = sshmsgchan;
	a[2].v = &m;
	a[3].op = CHANEND;

	for(;;){
		switch(alt(a)){
		case 0:
			path = fid->qid.path;
			switch(TYPE(path)){
			case Qcs:
				cs = fid->aux;
				if(cs){
					free(cs->resp);
					free(cs);
				}
				break;
			}
			if(fid->omode != -1 && TYPE(path) >= Qn)
				closeclient(client[NUM(path)]);
			sendp(fsclunkwaitchan, nil);
			break;
		case 1:
			switch(r->ifcall.type){
			case Tattach:
				fsattach(r);
				break;
			case Topen:
				fsopen(r);
				break;
			case Tread:
				fsread(r);
				break;
			case Twrite:
				fswrite(r);
				break;
			case Tstat:
				fsstat(r);
				break;
			case Tflush:
				fsflush(r);
				break;
			default:
				respond(r, "bug in fsthread");
				break;
			}
			sendp(fsreqwaitchan, 0);
			break;
		case 2:
			handlemsg(m);
			break;
		}
	}
}

static void
fssend(Req *r)
{
	sendp(fsreqchan, r);
	recvp(fsreqwaitchan);	/* avoids need to deal with spurious flushes */
}

static void
fsdestroyfid(Fid *fid)
{
	sendp(fsclunkchan, fid);
	recvp(fsclunkwaitchan);
}

void
takedown(Srv*)
{
	threadexitsall("done");
}

Srv fs = 
{
.attach=		fssend,
.destroyfid=	fsdestroyfid,
.walk1=		fswalk1,
.open=		fssend,
.read=		fssend,
.write=		fssend,
.stat=		fssend,
.flush=		fssend,
.end=		takedown,
};

void
threadmain(int argc, char **argv)
{
	int i, fd;
	char *host, *user, *p, *service;
	char *f[16];
	Msg *m;
	static Conn c;

	fmtinstall('B', mpfmt);
	fmtinstall('H', encodefmt);

	mtpt = "/net";
	service = nil;
	user = nil;
	ARGBEGIN{
	case 'B':	/* undocumented, debugging */
		doabort = 1;
		break;
	case 'D':	/* undocumented, debugging */
		debuglevel = strtol(EARGF(usage()), nil, 0);
		break;
	case '9':	/* undocumented, debugging */
		chatty9p++;
		break;

	case 'A':
		authlist = EARGF(usage());
		break;
	case 'c':
		cipherlist = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 's':
		service = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	host = argv[0];

	if((p = strchr(host, '@')) != nil){
		*p++ = '\0';
		user = host;
		host = p;
	}
	if(user == nil)
		user = getenv("user");
	if(user == nil)
		sysfatal("cannot find user name");

	privatefactotum();

	if((fd = dial(netmkaddr(host, "tcp", "ssh"), nil, nil, nil)) < 0)
		sysfatal("dialing %s: %r", host);

	c.interactive = isatty(0);
	c.fd[0] = c.fd[1] = fd;
	c.user = user;
	c.host = host;
	setaliases(&c, host);

	c.nokcipher = getfields(cipherlist, f, nelem(f), 1, ", ");
	c.okcipher = emalloc(sizeof(Cipher*)*c.nokcipher);
	for(i=0; i<c.nokcipher; i++)
		c.okcipher[i] = findcipher(f[i], allcipher, nelem(allcipher));

	c.nokauth = getfields(authlist, f, nelem(f), 1, ", ");
	c.okauth = emalloc(sizeof(Auth*)*c.nokauth);
	for(i=0; i<c.nokauth; i++)
		c.okauth[i] = findauth(f[i], allauth, nelem(allauth));

	sshclienthandshake(&c);

	requestpty(&c);		/* turns on TCP_NODELAY on other side */
	m = allocmsg(&c, SSH_CMSG_EXEC_SHELL, 0);
	sendmsg(m);

	time0 = time(0);
	sshmsgchan = chancreate(sizeof(Msg*), 16);
	fsreqchan = chancreate(sizeof(Req*), 0);
	fsreqwaitchan = chancreate(sizeof(void*), 0);
	fsclunkchan = chancreate(sizeof(Fid*), 0);
	fsclunkwaitchan = chancreate(sizeof(void*), 0);

	conn = &c;
	procrfork(sshreadproc, &c, 8192, RFNAMEG|RFNOTEG);
	procrfork(fsnetproc, nil, 8192, RFNAMEG|RFNOTEG);

	threadpostmountsrv(&fs, service, mtpt, MREPL);
	exits(0);
}

