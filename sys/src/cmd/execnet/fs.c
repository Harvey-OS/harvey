/*
 * ``Exec'' network device.  Mounted on net, provides /net/exec.
 *
 *	exec				protocol directory
 *		n 				connection directory
 *			ctl				control messages (like connect)
 *			data				data
 *			err				errors
 *			local				local address (pid of command)
 *			remote			remote address (command)
 *			status			status
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"

int fsdebug;

enum
{
	Qroot,
	Qexec,
	Qclone,
	Qn,
	Qctl,
	Qdata,
	Qlocal,
	Qremote,
	Qstatus,
};

#define PATH(type, n)	((type)|((n)<<8))
#define TYPE(path)		((int)(path) & 0xFF)
#define NUM(path)		((uint)(path)>>8)

typedef struct Tab Tab;
struct Tab
{
	char *name;
	ulong mode;
};

Tab tab[] =
{
	"/",		DMDIR|0555,
	"exec",	DMDIR|0555,
	"clone",	0666,
	nil,		DMDIR|0555,
	"ctl",		0666,
	"data",	0666,
	"local",	0444,
	"remote",	0444,
	"status",	0444,
};

void
setexecname(char *s)
{
	tab[Qexec].name = s;
}

ulong time0;

static void
fillstat(Dir *d, ulong path)
{
	Tab *t;
	int type;
	char buf[32];

	memset(d, 0, sizeof(*d));
	d->uid = estrdup("exec");
	d->gid = estrdup("exec");
	d->qid.path = path;
	d->atime = d->mtime = time0;
	d->length = 0;

	type = TYPE(path);
	t = &tab[type];
	if(t->name)
		d->name = estrdup(t->name);
	else{
		snprint(buf, sizeof buf, "%ud", NUM(path));
		d->name = estrdup(buf);
	}
	d->qid.type = t->mode>>24;
	d->mode = t->mode;
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
	if(i < 1){
		fillstat(d, PATH(Qexec, 0));
		return 0;
	}
	return -1;
}

static int
execgen(int i, Dir *d, void*)
{
	if(i < 1){
		fillstat(d, PATH(Qclone, 0));
		return 0;
	}
	i -= 1;

	if(i < nclient){
		fillstat(d, PATH(Qn, i));
		return 0;
	}
	return -1;
}

static int
conngen(int i, Dir *d, void *aux)
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

char *statusstr[] = 
{
	"Closed",
	"Exec",
	"Established",
	"Hangup",
};

static void
fsread(Req *r)
{
	char e[ERRMAX], *s;
	ulong path;

	path = r->fid->qid.path;
	switch(TYPE(path)){
	default:
		snprint(e, sizeof e, "bug in execnet path=%lux", path);
		respond(r, e);
		break;

	case Qroot:
		dirread9p(r, rootgen, nil);
		respond(r, nil);
		break;

	case Qexec:
		dirread9p(r, execgen, nil);
		respond(r, nil);
		break;

	case Qn:
		dirread9p(r, conngen, client[NUM(path)]);
		respond(r, nil);
		break;

	case Qctl:
		snprint(e, sizeof e, "%ud", NUM(path));
		readstr(r, e);
		respond(r, nil);
		break;

	case Qdata:
		dataread(r, client[NUM(path)]);
		break;

	case Qlocal:
		snprint(e, sizeof e, "%d", client[NUM(path)]->pid);
		readstr(r, e);
		respond(r, nil);
		break;

	case Qremote:
		s = client[NUM(path)]->cmd;
		if(strlen(s) >= 5)	/* "exec " */
			readstr(r, s+5);
		else
			readstr(r, s);
		respond(r, nil);
		break;

	case Qstatus:
		readstr(r, statusstr[client[NUM(path)]->status]);
		respond(r, nil);
		break;
	}
}

static void
fswrite(Req *r)
{
	char e[ERRMAX];
	ulong path;

	path = r->fid->qid.path;
	switch(TYPE(path)){
	default:
		snprint(e, sizeof e, "bug in execnet path=%lux", path);
		respond(r, e);
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
fsflush(Req *r)
{
	ulong path;
	Req *or;

	for(or=r; or->ifcall.type==Tflush; or=or->oldreq)
		;

	if(or->ifcall.type != Tread && or->ifcall.type != Twrite)
		abort();

	path = or->fid->qid.path;
	if(TYPE(path) != Qdata)
		abort();

	clientflush(or, client[NUM(path)]);
	respond(r, nil);
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
	char buf[32];
	int i, n;
	ulong path;

	if(!(fid->qid.type&QTDIR))
		return "walk in non-directory";

	path = fid->qid.path;
	if(strcmp(name, "..") == 0){
		switch(TYPE(path)){
		case Qn:
			qid->path = PATH(Qexec, 0);
			qid->type = QTDIR;
			return nil;
		case Qroot:
		case Qexec:
			qid->path = PATH(Qroot, 0);
			qid->type = QTDIR;
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
				qid->path = PATH(Qn, n);
				qid->type = QTDIR;
				return nil;
			}
			break;
		}
		if(strcmp(tab[i].name, name) == 0){
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
fsopen(Req *r)
{
	static int need[4] = { 4, 2, 6, 1 };
	ulong path;
	int n;
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
	case Qclone:
		n = newclient();
		path = PATH(Qctl, n);
		r->fid->qid.path = path;
		r->ofcall.qid.path = path;
		if(fsdebug)
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

Channel *cclunk;
Channel *cclunkwait;
Channel *creq;
Channel *creqwait;

static void
fsthread(void*)
{
	ulong path;
	Alt a[3];
	Fid *fid;
	Req *r;

	threadsetname("fsthread");

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
			if(fid->omode != -1 && TYPE(path) >= Qn)
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
fsdestroyfid(Fid *fid)
{
	sendp(cclunk, fid);
	recvp(cclunkwait);
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
};
