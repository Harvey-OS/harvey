/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>

#include "gconsole.h"

enum
{
	Maxfdata	= 8192,
	Maxiosize	= IOHDRSZ+Maxfdata,
};

enum {
	Qroot,
	Qcons,
	Qconsctl,
	Nqid,
};
static struct Qtab {
	char *name;
	int mode;
	int type;
	int length;
} qtab[Nqid] = {
	"/",
		DMDIR|0555,
		QTDIR,
		0,

	"cons",
		0666,
		0,
		0,

	"consctl",
		0222,
		0,
		0,
};

typedef struct Fid Fid;
struct Fid
{
	int32_t fd;
	Qid qid;
	int16_t opened;	/* -1 when not open */
	Fid *next;
};
static Fid *fids;
static Fid **ftail;

static int messagesize = Maxiosize;

/* no need to lock fids, we have only one mount point */
static Fid*
createFid(int32_t fd, Qid qid)
{
	Fid *fid;

	fid = (Fid*)mallocz(sizeof(Fid), 1);
	if(fid){
		fid->fd = fd;
		fid->qid = qid;
		fid->opened = -1;
		*ftail = fid;
		ftail = &fid->next;
	}
	return fid;
}
static Fid*
findFid(int32_t fd)
{
	Fid *fid;

	fid = fids;
	while(fid != nil && fid->fd != fd)
		fid = fid->next;
	return fid;
}

static int
readmessage(int fd, Fcall *req)
{
	int n;
	uint8_t	mdata[Maxiosize];

	n = read9pmsg(fd, mdata, sizeof mdata);
	if(n > 0)
		if(convM2S(mdata, n, req) == 0){
			debug("readmessage: convM2S returns 0\n");
			return -1;
		} else {
			return 1;
		}
	if(n < 0){
		debug("readmessage: read9pmsg: %r\n");
		return -1;
	}
	return 0;
}

static int
sendmessage(int fd, Fcall *rep)
{
	int n;
	uint8_t	mdata[Maxiosize];

	n = convS2M(rep, mdata, sizeof mdata);
	if(n == 0) {
		debug("sendmessage: convS2M error\n");
		return 0;
	}
	if(write(fd, mdata, n) != n) {
		debug("sendmessage: write\n");
		return 0;
	}
	return 1;
}
/* utilities */
static char *
invalidioreq(Fcall *req)
{
	if(req->count > Maxfdata || req->count < 0)
		return "bad read/write count";
	return nil;
}
static int
fillstat(uint64_t path, Dir *d)
{
	struct Qtab *t;

	memset(d, 0, sizeof(Dir));
	d->uid = "tty";
	d->gid = "tty";
	d->muid = "";
	d->qid = (Qid){path, 0, 0};
	d->atime = time(0);
	t = qtab + path;
	d->name = t->name;
	d->qid.type = t->type;
	d->mode = t->mode;
	d->length = t->length;
	return 1;
}
static int32_t
readtopdir(Fid *fid, uint8_t *buf, int64_t off, int32_t cnt, int blen)
{
	int32_t m, n;
	int64_t i, pos;
	Dir d;

	n = 0;
	pos = 0;
	for (i = 1; i < Nqid; i++){
		fillstat(i, &d);
		m = convD2M(&d, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	return n;
}
static void
rerror(Fcall *rep, char *err)
{
	debug("rerror: %s\n", err);
	rep->type = Rerror;
	rep->ename = err;
}
/* cons specific rread */
static void
readcons(int consreads, Fcall *req, Fcall *rep, char *data)
{
	int len;

	if((len = pread(consreads, data, req->count, req->offset)) < 0) {
		rerror(rep, "i/o error");
		return;
	}
	rep->type = Rread;
	rep->data = data;
	rep->count = len;
}
/* cons specific rwrite */
static void
writecons(int conswrites, Fcall *req, Fcall *rep)
{
	int len;

	if((len = pwrite(conswrites, req->data, req->count, req->offset)) < 0) {
		rerror(rep, "i/o error");
		return;
	}
	rep->type = Rwrite;
	rep->count = len;
}

/* 9p message handlers */
static void
rpermission(Fcall *req, Fcall *rep)
{
	rerror(rep, "permission denied");
}
static void
rattach(Fcall *req, Fcall *rep)
{
	char *spec;
	Fid *f;

	spec = req->aname;
	if(spec && spec[0]){
		rerror(rep, "bad attach specifier");
		return;
	}
	if(fids != nil){
		/* the first fid in fids must always be the attach one */
		rerror(rep, "device busy");
		return;
	}
	f = createFid(req->fid, (Qid){Qroot, 0, QTDIR});
	if(f == nil){
		rerror(rep, "out of memory");
	} else {
		rep->type = Rattach;
		rep->qid = f->qid;
	}
}
static void
rauth(Fcall *req, Fcall *rep)
{
	rerror(rep, "nconsole: authentication not required");
}
static void
rversion(Fcall *req, Fcall *rep)
{
	if(req->msize < 256){
		rerror(rep, "message size too small");
		return;
	}
	if(strncmp(req->version, "9P2000", 6) != 0){
		rerror(rep, "unrecognized 9P version");
		return;
	}
	messagesize = req->msize;
	if(messagesize > Maxiosize)
		messagesize = Maxiosize;
	rep->type = Rversion;
	rep->msize = messagesize;
	rep->version = "9P2000";
}
static void
rflush(Fcall *req, Fcall *rep)
{
	/* nothing to do */
	rep->type = Rflush;
}
static void
rwalk(Fcall *req, Fcall *rep)
{
	Fid *f, *n;
	Qid q;

	f = findFid(req->fid);
	if(f == nil){
		rerror(rep, "bad fid");
		return;
	}
	if(req->nwname > 1 || (req->nwname == 1 && f->qid.path != Qroot)){
		rerror(rep, "walk in non directory");
		return;
	}
	if(f->opened != -1){
		rerror(rep, "fid in use");
		return;
	}
	if(req->nwname == 1){
		if (strcmp(qtab[Qcons].name, req->wname[0]) == 0) {
			q = (Qid){Qcons, 0, 0};
		} else if (strcmp(qtab[Qconsctl].name, req->wname[0]) == 0) {
			q = (Qid){Qconsctl, 0, 0};
		} else {
			rerror(rep, "file does not exist");
			return;
		}
	} else {
		q = f->qid;
	}
	if(req->fid == req->newfid){
		n = f;
	} else {
		n = findFid(req->newfid);
		if(n == nil)
			n = createFid(req->newfid, q);
		else if(n->opened != -1){
			rerror(rep, "newfid already in use");
			return;
		}
		if(n == nil){
			rerror(rep, "out of memory");
			return;
		}
	}
	n->qid = q;
	rep->type = Rwalk;
	rep->nwqid = req->nwname;
	if(req->nwname)
		rep->wqid[0] = q;
}
static void
ropen(Fcall *req, Fcall *rep)
{
	static int need[4] = { 4, 2, 6, 1 };
	struct Qtab *t;
	Fid *f;
	int n;

	f = findFid(req->fid);
	if(f == nil){
		rerror(rep, "bad fid");
		return;
	}
	if(f->opened != -1){
		rerror(rep, "already open");
		return;
	}

	t = qtab + f->qid.path;
	n = need[req->mode & 3];
	if((n & t->mode) != n)
		rpermission(req, rep);
	else {
		f->opened = req->mode;
		rep->type = Ropen;
		rep->qid = f->qid;
		rep->iounit = 0;
	}
}
static void
rread(int consreads, Fcall *req, Fcall *rep, char *data)
{
	Fid *f;
	char *err;

	if(err = invalidioreq(req)){
		rerror(rep, err);
		return;
	}
	f = findFid(req->fid);
	if(f == nil){
		rerror(rep, "bad fid");
		return;
	}
	if(f->opened == -1 || (f->opened & 3) % 2 != 0){
		rerror(rep, "i/o error");
		return;
	}

	rep->type = Rread;
	if(f->qid.path == Qroot){
		rep->count = readtopdir(f, (uint8_t*)data, req->offset, req->count, Maxfdata);
		rep->offset = req->offset;
		rep->data = data + req->offset;
	} else if (f->qid.path == Qcons) {
		readcons(consreads, req, rep, data);
	} else {
		/* consctl */
		rpermission(req, rep);
	}
}
static void
rwrite(int conswrites, Fcall *req, Fcall *rep)
{
	Fid *f;
	char *err;

	if(err = invalidioreq(req)){
		rerror(rep, err);
		return;
	}
	f = findFid(req->fid);
	if(f == nil){
		rerror(rep, "bad fid");
		return;
	}
	if(f->opened == -1 || (f->opened & 3) != 1){
		rerror(rep, "i/o error");
		return;
	}

	/* cons writes do not come here */
	if(f->qid.path == Qroot){
		rpermission(req, rep);
	} else if (f->qid.path == Qcons) {
		writecons(conswrites, req, rep);
	} else {
		/* consctl */
		if(stdio){
			rerror(rep, "no raw mode from standard input");
			return;
		}
		if(strncmp("rawon", req->data, 5) == 0){
			rawmode = 1;
		} else if(strncmp("rawoff", req->data, 6) == 0){
			rawmode = 0;
		} else {
			rerror(rep, "unknown control message");
			return;
		}
		rep->type = Rwrite;
		rep->count = req->count;
	}
}
static void
rclunk(Fcall *req, Fcall *rep)
{
	Fid *f;

	f = findFid(req->fid);
	if(f == nil){
		rerror(rep, "bad fid");
		return;
	}

	f->opened = -1;
	rep->type = Rclunk;
}
static void
rstat(Fcall *req, Fcall *rep, char *data)
{
	Dir d;
	Fid *f;

	f = findFid(req->fid);
	if(f == nil){
		rerror(rep, "bad fid");
		return;
	}

	fillstat(f->qid.path, &d);
	rep->type = Rstat;
	rep->nstat = convD2M(&d, (uint8_t*)data, Maxfdata);
	rep->stat = (uint8_t*)data;
}

static void (*fcalls[])(Fcall *, Fcall *) = {
	[Tversion]	rversion,
	[Tflush]	rflush,
	[Tauth]		rauth,
	[Tattach]	rattach,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rpermission,
	[Tclunk]	rclunk,
	[Tremove]	rpermission,
	[Twstat]	rpermission,
};

/* serve9p is the main loop
 *
 * keep it simple:
 *  - when clients read from /dev/cons we read from consreads
 *  - when clients write to /dev/cons we write to conswrites
 * otherwise we handle the request locally.
 */
void
serve(int connection, int consreads, int conswrites)
{
	int pid, r, w;
	char data[Maxfdata];
	Fcall	rep;
	Fcall	*req;

	pid = getpid();
	req = malloc(sizeof(Fcall)+Maxfdata);
	if(req == nil)
		sysfatal("out of memory");

	ftail = &fids;

	debug("serve9p %d: started\n", pid);

	for(;;){
		if((r = readmessage(connection, req)) <= 0){
			debug("serve9p %d: readmessage returns %d\n", pid, r);
			break;
		}
		debug("serve9p %d: <-%F\n", pid, req);

		if(req->type < Tversion || req->type > Twstat) {
			rep.type = Rerror;
			rep.ename = "bad fcall type";
		} else {
			rep.tag = req->tag;
			switch(req->type){
				case Tread:
					rread(consreads, req, &rep, data);
					break;
				case Tstat:
					rstat(req, &rep, data);
					break;
				case Twrite:
					rwrite(conswrites, req, &rep);
					break;
				default:
					(*fcalls[req->type])(req, &rep);
					break;
			}
		}

		debug("serve9p %d: ->%F\n", pid, &rep);

		if((w = sendmessage(connection, &rep)) <= 0){
			debug("serve9p %d: sendmessage returns %d\n", pid, w);
			break;
		}
	}

	free(req);

	/* signal our friends that we finished */
	close(connection);
	debug("serve9p %d: close(%d)\n", pid, connection);
	close(consreads);
	debug("serve9p %d: close(%d)\n", pid, consreads);
	close(conswrites);
	debug("serve9p %d: close(%d)\n", pid, conswrites);

	debug("%s %d: shut down\n", argv0, pid);
	if(r < 0)
		sysfatal("serve9p: readmessage");
	if(w < 0)
		sysfatal("serve9p: sendmessage");
	exits(nil);
}
