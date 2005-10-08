/*
 * Web file system.  Conventionally mounted at /mnt/web
 *
 *	ctl				send control messages (might go away)
 *	cookies			list of cookies, editable
 *	clone			open and read to obtain new connection
 *	n				connection directory
 *		ctl				control messages (like get url)
 *		body				retrieved data
 *		content-type		mime content-type of body
 *		postbody			data to be posted
 *		parsed			parsed version of url
 * 			url				entire url
 *			scheme			http, ftp, etc.
 *			host				hostname
 *			path				path on host
 *			query			query after path
 *			fragment			#foo anchor reference
 *			user				user name (ftp)
 *			password			password (ftp)
 *			ftptype			transfer mode (ftp)
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

int fsdebug;

enum
{
	Qroot,
	Qrootctl,
	Qclone,
	Qcookies,
	Qclient,
	Qctl,
	Qbody,
	Qbodyext,
	Qcontenttype,
	Qpostbody,
	Qparsed,
	Qurl,
	Qscheme,
	Qschemedata,
	Quser,
	Qpasswd,
	Qhost,
	Qport,
	Qpath,
	Qquery,
	Qfragment,
	Qftptype,
	Qend,
};

#define PATH(type, n)	((type)|((n)<<8))
#define TYPE(path)		((int)(path) & 0xFF)
#define NUM(path)		((uint)(path)>>8)

Channel *creq;
Channel *creqwait;
Channel *cclunk;
Channel *cclunkwait;

typedef struct Tab Tab;
struct Tab
{
	char *name;
	ulong mode;
	int offset;
};

Tab tab[] =
{
	"/",			DMDIR|0555,		0,
	"ctl",			0666,			0,
	"clone",		0666,			0,
	"cookies",		0666,			0,
	"XXX",		DMDIR|0555,		0,
	"ctl",			0666,			0,
	"body",		0444,			0,
	"XXX",		0444,			0,
	"contenttype",	0444,			0,
	"postbody",	0666,			0,
	"parsed",		DMDIR|0555,		0,
	"url",			0444,			offsetof(Url, url),
	"scheme",		0444,			offsetof(Url, scheme),
	"schemedata",	0444,			offsetof(Url, schemedata),
	"user",		0444,			offsetof(Url, user),
	"passwd",		0444,			offsetof(Url, passwd),
	"host",		0444,			offsetof(Url, host),
	"port",		0444,			offsetof(Url, port),
	"path",		0444,			offsetof(Url, path),
	"query",		0444,			offsetof(Url, query),
	"fragment",	0444,			offsetof(Url, fragment),
	"ftptype",		0444,			offsetof(Url, ftp.type),
};

ulong time0;

static void
fillstat(Dir *d, uvlong path, ulong length, char *ext)
{
	Tab *t;
	int type;
	char buf[32];

	memset(d, 0, sizeof(*d));
	d->uid = estrdup("web");
	d->gid = estrdup("web");
	d->qid.path = path;
	d->atime = d->mtime = time0;
	d->length = length;
	type = TYPE(path);
	t = &tab[type];
	if(type == Qbodyext) {
		snprint(buf, sizeof buf, "body.%s", ext == nil ? "xxx" : ext);
		d->name = estrdup(buf);
	}
	else if(t->name)
		d->name = estrdup(t->name);
	else{	/* client directory */
		snprint(buf, sizeof buf, "%ud", NUM(path));
		d->name = estrdup(buf);
	}
	d->qid.type = t->mode>>24;
	d->mode = t->mode;
}

static void
fsstat(Req *r)
{
	fillstat(&r->d, r->fid->qid.path, 0, nil);
	respond(r, nil);
}

static int
rootgen(int i, Dir *d, void*)
{
	char buf[32];

	i += Qroot+1;
	if(i < Qclient){
		fillstat(d, i, 0, nil);
		return 0;
	}
	i -= Qclient;
	if(i < nclient){
		fillstat(d, PATH(Qclient, i), 0, nil);
		snprint(buf, sizeof buf, "%d", i);
		free(d->name);
		d->name = estrdup(buf);
		return 0;
	}
	return -1;
}

static int
clientgen(int i, Dir *d, void *aux)
{
	Client *c;

	c = aux;
	i += Qclient+1;
	if(i <= Qparsed){
		fillstat(d, PATH(i, c->num), 0, c->ext);
		return 0;
	}
	return -1;
}

static int
parsedgen(int i, Dir *d, void *aux)
{
	Client *c;

	c = aux;
	i += Qparsed+1;
	if(i < Qend){
		fillstat(d, PATH(i, c->num), 0, nil);
		return 0;
	}
	return -1;
}

static void
fsread(Req *r)
{
	char *s;
	char e[ERRMAX];
	Client *c;
	ulong path;

	path = r->fid->qid.path;
	switch(TYPE(path)){
	default:
		snprint(e, sizeof e, "bug in webfs path=%lux\n", path);
		respond(r, e);
		break;

	case Qroot:
		dirread9p(r, rootgen, nil);
		respond(r, nil);
		break;

	case Qrootctl:
		globalctlread(r);
		break;

	case Qcookies:
		cookieread(r);
		break;

	case Qclient:
		dirread9p(r, clientgen, client[NUM(path)]);
		respond(r, nil);
		break;

	case Qctl:
		ctlread(r, client[NUM(path)]);
		break;

	case Qcontenttype:
		c = client[NUM(path)];
		if(c->contenttype == nil)
			r->ofcall.count = 0;
		else
			readstr(r, c->contenttype);
		respond(r, nil);
		break;

	case Qpostbody:
		c = client[NUM(path)];
		readbuf(r, c->postbody, c->npostbody);
		respond(r, nil);
		break;
		
	case Qbody:
	case Qbodyext:
		c = client[NUM(path)];
		if(c->iobusy){
			respond(r, "already have i/o pending");
			break;
		}
		c->iobusy = 1;
		sendp(c->creq, r);
		break;

	case Qparsed:
		dirread9p(r, parsedgen, client[NUM(path)]);
		respond(r, nil);
		break;

	case Qurl:
	case Qscheme:
	case Qschemedata:
	case Quser:
	case Qpasswd:
	case Qhost:
	case Qport:
	case Qpath:
	case Qquery:
	case Qfragment:
	case Qftptype:
		c = client[NUM(path)];
		r->ofcall.count = 0;
		if(c->url != nil
		&& (s = *(char**)((uintptr)c->url+tab[TYPE(path)].offset)) != nil)
			readstr(r, s);
		respond(r, nil);
		break;
	}
}

static void
fswrite(Req *r)
{
	int m;
	ulong path;
	char e[ERRMAX], *buf, *cmd, *arg;
	Client *c;

	path = r->fid->qid.path;
	switch(TYPE(path)){
	default:
		snprint(e, sizeof e, "bug in webfs path=%lux\n", path);
		respond(r, e);
		break;

	case Qcookies:
		cookiewrite(r);
		break;

	case Qrootctl:
	case Qctl:
		if(r->ifcall.count >= 1024){
			respond(r, "ctl message too long");
			return;
		}
		buf = estredup(r->ifcall.data, (char*)r->ifcall.data+r->ifcall.count);
		cmd = buf;
		arg = strpbrk(cmd, "\t ");
		if(arg){
			*arg++ = '\0';
			arg += strspn(arg, "\t ");
		}else
			arg = "";
		r->ofcall.count = r->ifcall.count;
		if(TYPE(path)==Qrootctl){
			if(!ctlwrite(r, &globalctl, cmd, arg)
			&& !globalctlwrite(r, cmd, arg))
				respond(r, "unknown control command");
		}else{
			c = client[NUM(path)];
			if(!ctlwrite(r, &c->ctl, cmd, arg)
			&& !clientctlwrite(r, c, cmd, arg))
				respond(r, "unknown control command");
		}
		free(buf);
		break;

	case Qpostbody:
		c = client[NUM(path)];
		if(c->bodyopened){
			respond(r, "cannot write postbody after opening body");
			break;
		}
		if(r->ifcall.offset >= 128*1024*1024){	/* >128MB is probably a mistake */
			respond(r, "offset too large");
			break;
		}
		m = r->ifcall.offset + r->ifcall.count;
		if(c->npostbody < m){
			c->postbody = erealloc(c->postbody, m);
			memset(c->postbody+c->npostbody, 0, m-c->npostbody);
			c->npostbody = m;
		}
		memmove(c->postbody+r->ifcall.offset, r->ifcall.data, r->ifcall.count);
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		break;
	}
}

static void
fsopen(Req *r)
{
	static int need[4] = { 4, 2, 6, 1 };
	ulong path;
	int n;
	Client *c;
	Tab *t;

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
	case Qcookies:
		cookieopen(r);
		break;

	case Qpostbody:
		c = client[NUM(path)];
		c->havepostbody++;
		c->ref++;
		respond(r, nil);
		break;

	case Qbody:
	case Qbodyext:
		c = client[NUM(path)];
		if(c->url == nil){
			respond(r, "url is not yet set");
			break;
		}
		c->bodyopened = 1;
		c->ref++;
		sendp(c->creq, r);
		break;

	case Qclone:
		n = newclient(0);
		path = PATH(Qctl, n);
		r->fid->qid.path = path;
		r->ofcall.qid.path = path;
		if(fsdebug)
			fprint(2, "open clone => path=%lux\n", path);
		t = &tab[Qctl];
		/* fall through */
	default:
		if(t-tab >= Qclient)
			client[NUM(path)]->ref++;
		respond(r, nil);
		break;
	}
}

static void
fsdestroyfid(Fid *fid)
{
	sendp(cclunk, fid);
	recvp(cclunkwait);
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

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i, n;
	ulong path;
	char buf[32], *ext;

	path = fid->qid.path;
	if(!(fid->qid.type&QTDIR))
		return "walk in non-directory";

	if(strcmp(name, "..") == 0){
		switch(TYPE(path)){
		case Qparsed:
			qid->path = PATH(Qclient, NUM(path));
			qid->type = tab[Qclient].mode>>24;
			return nil;
		case Qclient:
		case Qroot:
			qid->path = PATH(Qroot, 0);
			qid->type = tab[Qroot].mode>>24;
			return nil;
		default:
			return "bug in fswalk1";
		}
	}

	i = TYPE(path)+1;
	for(; i<nelem(tab); i++){
		if(i==Qclient){
			n = atoi(name);
			snprint(buf, sizeof buf, "%d", n);
			if(n < nclient && strcmp(buf, name) == 0){
				qid->path = PATH(i, n);
				qid->type = tab[i].mode>>24;
				return nil;
			}
			break;
		}
		if(i==Qbodyext){
			ext = client[NUM(path)]->ext;
			snprint(buf, sizeof buf, "body.%s", ext == nil ? "xxx" : ext);
			if(strcmp(buf, name) == 0){
				qid->path = PATH(i, NUM(path));
				qid->type = tab[i].mode>>24;
				return nil;
			}
		}
		else if(strcmp(name, tab[i].name) == 0){
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
fsflush(Req *r)
{
	Req *or;
	int t;
	Client *c;
	ulong path;

	or=r;
	while(or->ifcall.type==Tflush)
		or = or->oldreq;

	if(or->ifcall.type != Tread && or->ifcall.type != Topen)
		abort();

	path = or->fid->qid.path;
	t = TYPE(path);
	if(t != Qbody && t != Qbodyext)
		abort();

	c = client[NUM(path)];
	sendp(c->creq, r);
	iointerrupt(c->io);
}

static void
fsthread(void*)
{
	ulong path;
	Alt a[3];
	Fid *fid;
	Req *r;

	threadsetname("fsthread");
	plumbstart();

	a[0].op = CHANRCV;
	a[0].c = cclunk;
	a[0].v = &fid;
	a[1].op = CHANRCV;
	a[1].c = creq;
	a[1].v = &r;
	a[2].op = CHANEND;

	for(;;){
		switch(alt(a)){
		case 0:
			path = fid->qid.path;
			if(TYPE(path)==Qcookies)
				cookieclunk(fid);
			if(fid->omode != -1 && TYPE(path) >= Qclient)
				closeclient(client[NUM(path)]);
			sendp(cclunkwait, nil);
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
			sendp(creqwait, 0);
			break;
		}
	}
}

static void
fssend(Req *r)
{
	sendp(creq, r);
	recvp(creqwait);	/* avoids need to deal with spurious flushes */
}

void
initfs(void)
{
	time0 = time(0);
	creq = chancreate(sizeof(void*), 0);
	creqwait = chancreate(sizeof(void*), 0);
	cclunk = chancreate(sizeof(void*), 0);
	cclunkwait = chancreate(sizeof(void*), 0);
	procrfork(fsthread, nil, STACK, RFNAMEG);
}

void
takedown(Srv*)
{
	closecookies();
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

