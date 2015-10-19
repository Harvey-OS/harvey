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

#include "console.h"

enum
{
	Maxfdata	= IODATASZ,
	Miniosize	= IOHDRSZ+ScreenBufferSize,
	Maxiosize	= IOHDRSZ+Maxfdata,
};

typedef enum
{
	Initializing,
	Mounted,
	Unmounted,		/* serve() loop continue status < Unmounted */
} Status;

static Status status;
static int rawmode;

static char *filesowner;
static void *data;

enum {
	Qroot,
	Qcons,
	Qconsctl,
	Nqid,
	Qinput,
	Qoutput,
	Nhqid,
};
static struct Qtab {
	char *name;
	int mode;
	int type;
	int length;
} qtab[Nhqid] = {
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

	"",
		0,
		0,
		0,

	"gconsin",		/* data from input will be written here */
		DMAPPEND|DMEXCL|0222,
		0,
		0,

	"gconsout",		/* data to output will be read here */
		DMEXCL|0444,
		0,
		0,
};

/* message size for the exported name space
 *
 * between Miniosize and Maxiosize
 */
static int messagesize = Maxiosize;

/* Initialized on Qinput open */
static Buffer *input;	/* written on Twrite(Qinput), read on Tread(Qcons) */

/* Initialized on Qoutput open */
static Buffer *output;	/* written on Twrite(Qcons), read on Tread(Qoutput) */

/* linked list of known fids
 *
 * NOTE: we don't free() Fids, because there's no appropriate point
 *	in 9P2000 to do that, except the Tclunk of the attach fid,
 *	that in our case corresponds to shutdown
 *	(the kernel is our single client, we are doomed to trust it)
 */
typedef struct Fid Fid;
struct Fid
{
	int32_t fd;
	Qid qid;
	int16_t opened;	/* -1 when not open */
	Fid *next;
};
#define ISCLOSED(f) (f != nil && f->opened == -1)

static Fid *fids;
static Fid **ftail;
static Fid *external;	/* attach fid of the last mount() in gconsole.c */
static Fid *inputfid;	/* from Topen(Qinput) */
static Fid *outputfid;	/* from Topen(Qoutput) */

static Fid*
createFid(int32_t fd, Qid qid)
{
	Fid *fid;

	fid = (Fid*)malloc(sizeof(Fid));
	if(fid){
		fid->fd = fd;
		fid->qid = qid;
		fid->opened = -1;
		fid->next = nil;
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

/* utilities */
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

/* queue of pending reads */
typedef struct AsyncOp AsyncOp;
struct AsyncOp
{
	int32_t tag;
	AsyncOp *next;
};
typedef struct OpQueue OpQueue;
struct OpQueue
{
	AsyncOp *head;
	AsyncOp **tail;
	int32_t minsize;
};
#define qempty(q) (q->tail == &q->head)

static OpQueue consreads;
static OpQueue outputreads;

static void
qinit(OpQueue *q)
{
	q->tail = &q->head;
	q->minsize = Maxfdata;
}
static int
enqueue(OpQueue *queue, int32_t tag, int32_t count)
{
	AsyncOp *read;

	debug("enqueue(%#p, %d, %d) queue->minsize = %d\n", queue, tag, count, queue->minsize);
	read = (AsyncOp*)malloc(sizeof(AsyncOp));
	if(!read)
		 return 0;
	if(count < queue->minsize)
		 queue->minsize = count;
	read->tag = tag;
	read->next = nil;
	*queue->tail = read;
	queue->tail = &read->next;
	return 1;
}
static void
dequeue(OpQueue *queue, int32_t tag)
{
	AsyncOp *op, *prev;

	if(qempty(queue))
		return;

	prev = nil;
	op = queue->head;
	while(op && op->tag != tag){
		prev = op;
		op = op->next;
	}
	if(op == nil)
		return;
	if(prev == nil)
		queue->head = op->next;
	else
		prev->next = op->next;
	if(queue->tail == &op->next)
		queue->tail = &prev->next;
	else
		queue->tail = &queue->head;
	free(op);
}
static int
sync(int connection, Fcall *rep, OpQueue *queue, Buffer *data)
{
	AsyncOp *op;
	int w;

	if(qempty(queue) || bempty(data))
		return 1;

	debug("sync(queue = %#p, data = %#p), queue->minsize = %d\n", queue, data, queue->minsize);
	rep->type = Rread;
	rep->count = queue->minsize;
	rep->data = bread(data, &rep->count);

	op = queue->head;
	while(op != nil){
		queue->head = op->next;
		rep->tag = op->tag;

		w = sendmessage(connection, rep);
		if(w <= 0){
			/* we had an error on the connection: stop to serve() */
			debug("serve9p %d: sync: %d bytes ready, but sendmessage returns %d\n", getpid(), rep->count, w);
			return w;
		}

		free(op);
		op = queue->head;
	}
	queue->tail = &queue->head;
	queue->minsize = Maxfdata;
	return 1;
}
static int
qclose(int connection, Fcall *rep, OpQueue *queue)
{
	AsyncOp *op;
	int w;

	if(qempty(queue))
		return 1;

	debug("sync(queue = %#p), queue->minsize = %d\n", queue, data, queue->minsize);
	rep->type = Rread;
	rep->count = 0;
	rep->data = nil;

	op = queue->head;
	while(op != nil){
		queue->head = op->next;
		rep->tag = op->tag;

		w = sendmessage(connection, rep);
		if(w <= 0){
			/* we had an error on the connection: stop to serve() */
			debug("serve9p %d: qclose: sendmessage returns %d\n", getpid(), w);
			return w;
		}

		free(op);
		op = queue->head;
	}
	queue->tail = &queue->head;
	queue->minsize = Maxfdata;
	return 1;
}

/* 9p message handlers */
static char *
invalidioreq(Fcall *req)
{
	if(req->count > messagesize || req->count < 0)
		return "bad read/write count";
	return nil;
}
static int
fillstat(uint64_t path, Dir *d)
{
	struct Qtab *t;

	debug("fillstat: filesowner = %s", filesowner);
	memset(d, 0, sizeof(Dir));
	d->uid = filesowner;
	d->gid = filesowner;
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
rootread(Fid *fid, uint8_t *buf, int64_t off, int32_t cnt, int blen)
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
static int
rerror(Fcall *rep, char *err)
{
	debug("rerror: %s\n", err);
	rep->type = Rerror;
	rep->ename = err;
	return 1;
}
static int
rpermission(Fcall *req, Fcall *rep)
{
	return rerror(rep, "permission denied");
}
static int
rattach(Fcall *req, Fcall *rep)
{
	char *spec;
	Fid *f;

	spec = req->aname;
	if(spec && spec[0])
		return rerror(rep, "bad attach specifier");

	if(external != nil){
		/* we expect 3 valid Tattach:
		 * 1 for the process that will send us the input, writing Qinput
		 * 1 for the process that will print our output, reading Qoutput
		 * 1 for the rest of the children
		 */
		return rerror(rep, "device busy");
	}
	f = findFid(req->fid);
	if(f == nil)
		f = createFid(req->fid, (Qid){Qroot, 0, QTDIR});
	if(f == nil)
		return rerror(rep, "out of memory");

	if(input != nil && output != nil){
		external = f;
		status = Mounted;
	}

	rep->type = Rattach;
	rep->qid = f->qid;
	return 1;
}
static int
rauth(Fcall *req, Fcall *rep)
{
	return rerror(rep, "nconsole: authentication not required");
}
static int
rversion(Fcall *req, Fcall *rep)
{
	if(data != nil)
		return rerror(rep, "device busy");
	if(req->msize < Miniosize)
		return rerror(rep, "message size too small");
	if(strncmp(req->version, "9P2000", 6) != 0)
		return rerror(rep, "unrecognized 9P version");

	messagesize = req->msize;
	if(messagesize > Maxiosize)
		messagesize = Maxiosize;
	data = malloc(messagesize);
	rep->type = Rversion;
	rep->msize = messagesize;
	rep->version = "9P2000";
	return 1;
}
static int
rflush(Fcall *req, Fcall *rep)
{
	dequeue(&consreads, req->oldtag);
	dequeue(&outputreads, req->oldtag);

	rep->type = Rflush;
	return 1;
}
static int
rwalk(Fcall *req, Fcall *rep)
{
	Fid *f, *n;
	Qid q;

	f = findFid(req->fid);
	if(f == nil)
		return rerror(rep, "bad fid");
	if(req->nwname > 1 || (req->nwname == 1 && f->qid.path != Qroot))
		return rerror(rep, "walk in non directory");
	if(f->opened != -1)
		return rerror(rep, "fid in use");

	if(req->nwname == 1){
		if (strcmp(qtab[Qcons].name, req->wname[0]) == 0) {
			q = (Qid){Qcons, 0, 0};
		} else if (strcmp(qtab[Qconsctl].name, req->wname[0]) == 0) {
			q = (Qid){Qconsctl, 0, 0};
		} else if (!input && strcmp(qtab[Qinput].name, req->wname[0]) == 0) {
			q = (Qid){Qinput, 0, 0};
		} else if (!output && strcmp(qtab[Qoutput].name, req->wname[0]) == 0) {
			q = (Qid){Qoutput, 0, 0};
		} else {
			return rerror(rep, "file does not exist");
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
		else if(n->opened != -1)
			return rerror(rep, "newfid already in use");

		if(n == nil)
			return rerror(rep, "out of memory");
	}
	n->qid = q;
	rep->type = Rwalk;
	rep->nwqid = req->nwname;
	if(req->nwname)
		rep->wqid[0] = q;
	return 1;
}
static int
ropen(Fcall *req, Fcall *rep)
{
	static int need[4] = { 4, 2, 6, 1 };
	struct Qtab *t;
	Fid *f;
	int n;

	f = findFid(req->fid);
	if(f == nil)
		return rerror(rep, "bad fid");
	if(f->opened != -1)
		return rerror(rep, "already open");

	t = qtab + f->qid.path;
	n = need[req->mode & 3];
	if((n & t->mode) != n)
		return rpermission(req, rep);

	switch(f->qid.path)
	{
		case Qinput:
			inputfid = f;
			if(stdio){
				input = balloc(Maxfdata);
			} else {
				input = balloc(1024); /* who type so fast? */
				input->flushlines = 1;
			}
			break;
		case Qoutput:
			outputfid = f;
			output = balloc(Maxfdata);
			break;
		case Qcons:
			if(ISCLOSED(inputfid) && (req->mode & OREAD) == OREAD)
				return rerror(rep, "input device closed");
			else if(ISCLOSED(outputfid) && (req->mode & OWRITE) == OWRITE)
				return rerror(rep, "output device closed");
			break;
		default:
			break;
	}
	f->opened = req->mode;
	rep->type = Ropen;
	rep->qid = f->qid;
	rep->iounit = 0;
	return 1;
}
static int
rread(Fcall *req, Fcall *rep)
{
	Fid *f;
	char *err;

	if(err = invalidioreq(req))
		return rerror(rep, err);

	f = findFid(req->fid);
	if(f == nil)
		return rerror(rep, "bad fid");
	if(ISCLOSED(f) || (f->opened & 3) % 2 != 0)
		return rerror(rep, "i/o error");

	rep->type = Rread;
	if(req->count == 0){
		rep->count = 0;
		rep->data = nil;
		return 1;
	}
	switch(f->qid.path){
		case Qroot:
			rep->count = rootread(f, (uint8_t*)data, req->offset, req->count, Maxfdata);
			rep->data = data + req->offset;
			return 1;
		case Qcons:
			if(ISCLOSED(inputfid)){
				rep->count = 0;
				rep->data = nil;
				return 1;
			}
			if(!enqueue(&consreads, req->tag, req->count))
				return rerror(rep, "out of memory");
			return 0;
		case Qoutput:
			if(!enqueue(&outputreads, req->tag, req->count))
				return rerror(rep, "out of memory");
			return 0;
		default:
			return rerror(rep, "i/o error");
	}
}
static int
rwrite(Fcall *req, Fcall *rep)
{
	int prev;
	Fid *f;
	char *err;

	if(err = invalidioreq(req))
		return rerror(rep, err);

	f = findFid(req->fid);
	if(f == nil)
		return rerror(rep, "bad fid");
	if(ISCLOSED(f) || (f->opened & OWRITE) != OWRITE)
		return rerror(rep, "i/o error");

	switch(f->qid.path){
		case Qcons:
			if(ISCLOSED(outputfid))
				rep->count = 0;
			else
				rep->count = bwrite(output, req->data, req->count);
			break;
		case Qconsctl:
			if(blind)
				return rerror(rep, "no raw mode when blind");

			if(strncmp("rawon", req->data, 5) == 0)
				rawmode = 1;
			else if(strncmp("rawoff", req->data, 6) == 0)
				rawmode = 0;
			else
				return rerror(rep, "unknown control message");
			input->flushlines = 1;
			rep->count = req->count;
			break;
		case Qinput:
			rep->count = bwrite(input, req->data, req->count);
			if(!rawmode && !blind && rep->count){
				/* send a feedback to the user */
				prev = rep->count;
				rep->count = bwrite(output, req->data, rep->count);
				if(prev > rep->count)
					bdelete(output, prev - rep->count);
			}
			break;
		default:
			return rerror(rep, "i/o error");
	}
	rep->type = Rwrite;
	return 1;
}
static int
rclunk(Fcall *req, Fcall *rep)
{
	Fid *f;

	f = findFid(req->fid);
	if(f == nil)
		return rerror(rep, "bad fid");

	if(f == external){
		debug("serve9p: external clients gone\n");
		status = Unmounted;
	}

	f->opened = -1;
	rep->type = Rclunk;
	return 1;
}
static int
rstat(Fcall *req, Fcall *rep)
{
	Dir d;
	Fid *f;

	f = findFid(req->fid);
	if(f == nil || f->qid.path >= Nqid)
		return rerror(rep, "bad fid");

	fillstat(f->qid.path, &d);
	rep->type = Rstat;
	rep->nstat = convD2M(&d, (uint8_t*)data, messagesize);
	rep->stat = (uint8_t*)data;
	return 1;
}

static int (*fcalls[])(Fcall *, Fcall *) = {
	[Tversion]	rversion,
	[Tauth]		rauth,
	[Tattach]	rattach,
	[Tflush]	rflush,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rpermission,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rpermission,
	[Tstat]		rstat,
	[Twstat]	rpermission,
};

int
fsinit(int *mnt, int *mntdev)
{
	int tmp[2];

	pipe(tmp);
	*mnt = tmp[0];
	*mntdev = 'M';

	return tmp[1];
}

/* serve9p is the main loop */
void
fsserve(int connection, char *owner)
{
	int pid, r, w, syncrep;
	Fcall	rep;
	Fcall	*req;

	if(owner == nil)
		sysfatal("owner undefined");
	filesowner = strdup(owner);

	pid = getpid();
	req = malloc(sizeof(Fcall)+Maxfdata);
	if(req == nil)
		sysfatal("out of memory");

	ftail = &fids;
	qinit(&consreads);
	qinit(&outputreads);

	status = Initializing;

	debug("serve9p %d: started\n", pid);

	do
	{
		if((r = readmessage(connection, req)) <= 0){
			debug("serve9p %d: readmessage returns %d\n", pid, r);
			break;
		}
		debug("serve9p %d: <-%F\n", pid, req);

		rep.tag = req->tag;
		if(req->type < Tversion || req->type > Twstat)
			syncrep = rerror(&rep, "bad fcall type");
		else
			syncrep = (*fcalls[req->type])(req, &rep);

		if(syncrep)
			debug("serve9p %d: ->%F\n", pid, &rep);

		if(syncrep && (w = sendmessage(connection, &rep)) <= 0){
			debug("serve9p %d: sendmessage returns %d\n", pid, w);
			break;
		}

		if((status == Unmounted || ISCLOSED(outputfid)) && (w = qclose(connection, &rep, &outputreads)) <= 0){
			debug("serve9p %d: qclose(outputreads) returns %d\n", pid, w);
			break;
		} else if((w = sync(connection, &rep, &outputreads, output)) <= 0){
			debug("serve9p %d: sync(outputreads, output) returns %d\n", pid, w);
			break;
		}
		if((status == Unmounted || ISCLOSED(inputfid)) && (w = qclose(connection, &rep, &consreads)) <= 0){
			debug("serve9p %d: qclose(consreads) returns %d\n", pid, w);
			break;
		} else if((w = sync(connection, &rep, &consreads, input)) <= 0){
			debug("serve9p %d: sync(consreads, input) returns %d\n", pid, w);
			break;
		}

		/* We can exit (properly) only when the following conditions hold
		 *
		 * - the kernel decided that nobody need us anymore
		 *   (status == Unmounted, see rclunk and rattach)
		 * - the inputfid has been closed (so that the Qinput writer
		 *   exited, releasing the actual input device)
		 * - the outputfid has been closed (so that the Qoutput reader
		 *   exited, releasing the actual output device)
		 *
		 * Even if no processes in the namespace are currently using
		 * cons or consctl, as far as the namespace exists one of its
		 * processes could still open cons (or consctl).
		 *
		 * Thus we exit when the kernel decides that nobody will
		 * need our services (aka, all the children sharing the
		 * mountpoint that we serve have exited).
		 *
		 * (AND obviously if an unexpected error occurred)
		 */
	}
	while(status < Unmounted || !ISCLOSED(inputfid) || !ISCLOSED(outputfid));


	if(r < 0)
		sysfatal("serve9p: readmessage");
	if(w < 0)
		sysfatal("serve9p: sendmessage");

	close(connection);
	debug("serve9p %d: close(%d)\n", pid, connection);

	debug("serve9p %d: shut down\n", pid);

	exits(nil);
}
