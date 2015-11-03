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
	Unmounted,		/* fsserve() loop while status < Unmounted */
} Status;

int systemwide;

static Status status;
static int rawmode;

static int fspid;
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

static uint32_t nextoutputcol;

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

	n = read9pmsg(fd, data, Maxiosize);
	if(n > 0)
		if(convM2S(data, n, req) == 0){
			debug("readmessage: convM2S returns 0\n");
			return -1;
		} else {
			debug("serve9p %d: <-%F\n", fspid, req);
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

	n = convS2M(rep, data, Maxiosize);
	if(n == 0) {
		debug("sendmessage: convS2M error\n");
		return 0;
	}
	if(write(fd, data, n) != n) {
		debug("sendmessage: write\n");
		return 0;
	}
	debug("serve9p %d: ->%F\n", fspid, rep);
	return 1;
}

/* queue of pending reads */
typedef struct AsyncOp AsyncOp;
struct AsyncOp
{
	uint32_t tag;
	uint32_t count;
	AsyncOp *next;
};
typedef struct OpQueue OpQueue;
struct OpQueue
{
	AsyncOp *head;
	AsyncOp **tail;
};
#define qempty(q) (q->tail == &q->head)

static OpQueue consreads;
static OpQueue conswrites;
static AsyncOp *outputread;		/* only one process can access Qoutput */

static void
qinit(OpQueue *q)
{
	q->tail = &q->head;
}
static AsyncOp *
opalloc(uint32_t tag, uint32_t count)
{
	AsyncOp *read;

	read = (AsyncOp*)malloc(sizeof(AsyncOp));
	if(!read)
		 return nil;
	read->tag = tag;
	read->count = count;
	read->next = nil;
	return read;
}
static int
enqueue(OpQueue *queue, uint32_t tag, uint32_t count)
{
	AsyncOp *read;

	debug("enqueue(%#p, %d, %d)\n", queue, tag, count);
	if((read = opalloc(tag, count)) == nil)
		return 0;
	*queue->tail = read;
	queue->tail = &read->next;
	return 1;
}
static void
dequeue(OpQueue *queue, uint32_t tag)
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
syncOutput(int connection, Fcall *rep)
{
	uint32_t l; //, i;
	int w;
	char *d;
	AsyncOp *aw;
	OpQueue *queue;

	if(outputread == nil || bempty(output))
		return 1;	/* continue */

	queue = &conswrites;
	l = outputread->count;
	d = bread(output, &l);

	rep->type = Rread;
	rep->tag = outputread->tag;
	rep->count = l;
	rep->data = d;

	w = sendmessage(connection, rep);
	if(w <= 0){
		/* we had an error on the connection: stop to fsserve() */
		debug("serve9p %d: syncOutput: %d bytes ready, but sendmessage returns %d\n", fspid, rep->count, w);
		return w;
	}

	free(outputread);
	outputread = nil;

	if(bempty(output)){
		w = 0;
		rep->type = Rwrite;
		while(!qempty(queue)){
			aw = queue->head;
			rep->tag = aw->tag;
			rep->count = aw->count;

			w = sendmessage(connection, rep);
			if(w <= 0){
				/* we had an error on the connection: stop to fsserve() */
				debug("serve9p %d: syncOutput: %d bytes written, but sendmessage returns %d\n", fspid, rep->count, w);
				return w;
			}

			queue->head = aw->next;
			if(queue->head == nil){
				queue->tail = &queue->head;
			}
			free(aw);
		}
	}

	return 1;
}
/* distribute available data to pending Tread(cons) */
static int
syncCons(int connection, Fcall *rep)
{
	AsyncOp *ar;
	OpQueue *queue;
	uint32_t l;
	int w;
	char *d;

	queue = &consreads;
	if(qempty(queue) || bempty(input))
		return 1;	/* continue */

	/* here we have a pending queue and either (checked by fscons)
	 *
	 * - a full line verified by blineready, or
	 * - a partial line with either rawmode active or linecontrol inactive
	 *
	 * in both case we have something to send and somebody ready to receive
	 */
	debug("syncCons(queue = %#p)\n", queue);
	rep->type = Rread;

	w = 0;
	while(!qempty(queue) && !bempty(input)){
		ar = queue->head;
		l = ar->count;
		if(rawmode)
			d = bread(input, &l);
		else
			d = breadln(input, &l);

		rep->tag = ar->tag;
		rep->count = l;
		rep->data = d;

		w = sendmessage(connection, rep);
		if(w <= 0){
			/* we had an error on the connection: stop to fsserve() */
			debug("serve9p %d: syncCons: %d bytes ready, but sendmessage returns %d\n", fspid, rep->count, w);
			return w;
		}

		queue->head = ar->next;
		if(queue->head == nil){
			queue->tail = &queue->head;
		}
		free(ar);
	}
	return 1;
}
static int
closeOutput(int connection, Fcall *rep)
{
	int w;

	if(!outputread)
		return 1;

	rep->type = Rread;
	rep->count = 0;
	rep->data = nil;
	rep->tag = outputread->tag;

	w = sendmessage(connection, rep);
	if(w <= 0){
		/* we had an error on the connection: stop to fsserve() */
		debug("serve9p %d: qclose: sendmessage returns %d\n", fspid, w);
		return w;
	}

	free(outputread);
	outputread = nil;

	return 1;
}
static int
closeConsReaders(int connection, Fcall *rep)
{
	OpQueue *queue;
	AsyncOp *op;
	int w;

	queue = &consreads;
	if(qempty(queue))
		return 1;

	debug("qclose(queue = %#p)\n", queue, data);
	rep->type = Rread;
	rep->count = 0;
	rep->data = nil;

	op = queue->head;
	while(op != nil){
		queue->head = op->next;
		rep->tag = op->tag;

		w = sendmessage(connection, rep);
		if(w <= 0){
			/* we had an error on the connection: stop to fsserve() */
			debug("serve9p %d: qclose: sendmessage returns %d\n", fspid, w);
			return w;
		}

		free(op);
		op = queue->head;
	}
	queue->tail = &queue->head;
	return 1;
}

/* line buffering and control */
static void
rawappender(char c, Buffer* b)
{
	b->data[b->written++] = c;
}
/* build the input line when linecontrol is on */
static void
lineprinter(char c, Buffer* b)
{
	switch(c){
		case '\n':
		case '\r':
			b->linewidth = 0;
			if(crnl){
				b->data[b->written++] = '\r';
				b->data[b->written++] = '\n';
				return;
			}
			break;
		case '\b':
			/* decrease line width a bit */
			if(b->linewidth)
				--b->linewidth;
			/* overwrite with a space */
			b->data[b->written++] = '\b';
			b->data[b->written++] = ' ';
			break;
		case '\t':
			/* turns \t to spaces */
			do
			{
				lineprinter(' ', b);
			}
			while(b->linewidth&7);
			return;
		default:
			++b->linewidth;
			break;
	}
	b->data[b->written++] = c;
}
/* build the input line when linecontrol is on
 *
 * note that this is only used for input, thus do not
 * compute linewidth, since \b would have variable width
 * according to the position (thanks \t!).
 */
static void
linebuilder(char c, Buffer* b)
{
	if (b->ctrld < b->size)
		return;	/* ignore everything after ^D, until it has been consumed */
	if(crnl && c == '\n' && b->written > 0 && b->data[b->written-1] == '\n')
		return; /* avoid duplications */

	switch(c){
		case '\004':	// ctrl-d
			b->ctrld = b->written;
			break;
		case 0x15:		// ctrl-u
			b->written = b->read;
			break;
		case 0x7f:		// backspace
		case '\b':
			if(!bempty(b))
				--b->written;
			break;
		case '\n':
			++b->newlines;	/* only newline count for us */
			b->data[b->written++] = c;
			break;
		case '\r':
			if(crnl){
				/* turn \r to \n */
				b->data[b->written++] = '\n';
				++b->newlines;
			} else {
				b->data[b->written++] = c;
			}
			break;
		default:
			b->data[b->written++] = c;
			break;
	}
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

	if(external != nil && !systemwide){
		/* when not system wide (aka screenconsole), we expect 3 valid Tattach:
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

	if(external == nil && input != nil && output != nil){
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
	return rerror(rep, "authentication not required");
}
static int
rversion(Fcall *req, Fcall *rep)
{
	if(req->msize < Miniosize)
		return rerror(rep, "message size too small");

	messagesize = req->msize;
	if(messagesize > Maxiosize)
		messagesize = Maxiosize;
	rep->type = Rversion;
	rep->msize = messagesize;
	if(*req->version == 0 || strncmp(req->version, "9P2000", 6) == 0)
		rep->version = "9P2000";
	else
		rep->version = "unknown";
	return 1;
}
static int
rflush(Fcall *req, Fcall *rep)
{
	if(req->tag == outputread->tag){
		free(outputread);
		outputread = nil;
	}else
		dequeue(&consreads, req->oldtag);

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
			if(linecontrol){
				input = balloc(Maxfdata);
				input->add = linebuilder;
			} else {
				input = balloc(Maxfdata);
				input->add = rawappender;
			}
			break;
		case Qoutput:
			outputfid = f;
			output = balloc(Maxfdata*4);	/* space for multiple verbose writers */
			if(linecontrol){
				output->add = lineprinter;
			} else {
				output->add = rawappender;
			}
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
			assert(outputread == nil);
			outputread = opalloc(req->tag, req->count);
			if(!outputread)
				return rerror(rep, "out of memory");
			return 0;
		default:
			return rerror(rep, "i/o error");
	}
}
static int
rwrite(Fcall *req, Fcall *rep)
{
	static char backspace = '\b';
	int maxlength, old, new;
	Fid *f;
	char *s;

	if(s = invalidioreq(req))
		return rerror(rep, s);

	f = findFid(req->fid);
	if(f == nil)
		return rerror(rep, "bad fid");
	if(ISCLOSED(f) || (f->opened & OWRITE) != OWRITE)
		return rerror(rep, "i/o error");

	switch(f->qid.path){
		case Qcons:
			if(ISCLOSED(outputfid))
				rep->count = 0;
			else if(bspace(output) < req->count * 2)
				return rerror(rep, "device busy");
			else {
				while(output->linewidth > nextoutputcol)
					bwrite(output, &backspace, 1);
				rep->count = bwrite(output, req->data, req->count);
				nextoutputcol = output->linewidth;
				if(!bempty(input))
					bwrite(output, input->data + input->read, input->written - input->read);
				if(bpending(output) > ScreenBufferSize * 3){
					/* the output buffer contains too much data:
					 * slow down writers by deferring Rwrites
					 */
					if(!enqueue(&conswrites, req->tag, rep->count))
						return rerror(rep, "out of memory");
					return 0;
				}
			}
			break;
		case Qconsctl:
			if(blind)
				return rerror(rep, "no raw mode when blind");

			if(strncmp("rawon", req->data, 5) == 0){
				input->ctrld = input->size;
				input->add = rawappender;
				output->add = rawappender;
				rawmode = 1;
			} else if(strncmp("rawoff", req->data, 6) == 0) {
				input->ctrld = input->size;
				input->add = linebuilder;
				output->add = lineprinter;
				rawmode = 0;
			} else
				return rerror(rep, "unknown control message");
			rep->count = req->count;
			break;
		case Qinput:
			if(ISCLOSED(outputfid) || status == Unmounted){
				rep->count = 0;
			} else if(rawmode || blind){
				rep->count = bwrite(input, req->data, req->count);
			} else if(!linecontrol) {
				/* life is easy:
				 *
				 * we have to give a feedback to the user but we do
				 * not have to handle control characters
				 */
				rep->count = bwrite(input, req->data, req->count);
				bwrite(output, req->data, rep->count);
			} else {
				/* we have to send a feedback to the user but also
				 * handle control characters
				 *
				 * we need enough spaces in both input and output
				 */
				maxlength = req->count;
				if((new = bspace(input)) < maxlength)
					maxlength = new;
				if((new = bspace(output) / 2) < maxlength)
					maxlength = new;

				old = bpending(input);
				rep->count = bwrite(input, req->data, maxlength);
				new = bpending(input);
				if(new == old + rep->count){
					/* no control characters */
					maxlength = bwrite(output, req->data, maxlength);

					/* we knew we have enough space, abort if not */
					assert(maxlength == rep->count);
				} else {
					/* sync visible output and input buffer
					 *
					 * we have to delete previously pending input chars
					 * from output, and then write them again
					 */
					while(output->linewidth > nextoutputcol)
						bwrite(output, &backspace, 1);
					if(new > 0)
						bwrite(output, input->data, new);
				}
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
	if(f != nil){
		if(f == external){
			debug("serve9p: external clients gone\n");
			status = Unmounted;
		}
		f->opened = -1;
	}
	rep->type = Rclunk;
	return 1;
}
static int
rstat(Fcall *req, Fcall *rep)
{
	Dir d;
	Fid *f;
	static uint8_t mdata[Maxiosize];

	f = findFid(req->fid);
	if(f == nil || f->qid.path >= Nqid)
		return rerror(rep, "bad fid");

	fillstat(f->qid.path, &d);
	rep->type = Rstat;
	rep->nstat = convD2M(&d, mdata, Maxiosize);
	rep->stat = mdata;
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
/* fsserve is the main loop */
void
fsserve(int connection, char *owner)
{
	int r, w, syncrep;
	Fcall	rep;
	Fcall	*req;

	if(owner == nil)
		sysfatal("owner undefined");
	filesowner = owner;

	fspid = getpid();
	req = malloc(sizeof(Fcall)+Maxfdata);
	if(req == nil)
		sysfatal("out of memory");
	data = malloc(messagesize);
	if(data == nil)
		sysfatal("out of memory");

	ftail = &fids;
	qinit(&consreads);
	qinit(&conswrites);

	status = Initializing;

	debug("serve9p %d: started\n", fspid);

	do
	{
		debug("serve9p %d: wait for a new request\n", fspid);
		if((r = readmessage(connection, req)) <= 0){
			debug("serve9p %d: readmessage returns %d\n", fspid, r);
			break;
		}

		rep.tag = req->tag;
		if(req->type < Tversion || req->type > Twstat)
			syncrep = rerror(&rep, "bad fcall type");
		else
			syncrep = (*fcalls[req->type])(req, &rep);

		if(syncrep && (w = sendmessage(connection, &rep)) <= 0){
			debug("serve9p %d: sendmessage returns %d\n", fspid, w);
			break;
		}

		/* first, display output to the user... */
		debug("serve9p %d: display output to the user... \n", fspid);
		if((status == Unmounted || ISCLOSED(outputfid)) && (w = closeOutput(connection, &rep)) <= 0){
			debug("serve9p %d: closeOutput returns %d\n", fspid, w);
			break;
		} else if((w = syncOutput(connection, &rep)) <= 0){
			debug("serve9p %d: syncOutput returns %d\n", fspid, w);
			break;
		}
		/* then distribute available input among cons readers... */
		debug("serve9p %d: distribute available input among cons readers... \n", fspid);
		if((status == Unmounted || ISCLOSED(inputfid)) && (w = closeConsReaders(connection, &rep)) <= 0){
			debug("serve9p %d: closeConsReaders returns %d\n", fspid, w);
			break;
		} else if(!linecontrol || rawmode || blineready(input)){
			if((w = syncCons(connection, &rep)) <= 0){
				debug("serve9p %d: syncCons(input) returns %d\n", fspid, w);
				break;
			}
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
	while(systemwide || status < Unmounted || !ISCLOSED(inputfid) || !ISCLOSED(outputfid));


	if(r < 0)
		sysfatal("serve9p: readmessage");
	if(w < 0)
		sysfatal("serve9p: sendmessage");

	close(connection);
	debug("serve9p %d: close(%d)\n", fspid, connection);

	debug("serve9p %d: shut down\n", fspid);

	exits(nil);
}
