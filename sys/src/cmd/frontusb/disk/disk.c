/*
 * usb/disk - usb mass storage file server
 *
 * supports only the scsi command interface, not ata.
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "scsireq.h"
#include "usb.h"
#include "ums.h"

enum
{
	Qdir = 0,
	Qctl,
	Qraw,
	Qdata,
	Qpart,
	Qmax = Maxparts,
};

Dev *dev;
Ums *ums;

typedef struct Dirtab Dirtab;
struct Dirtab
{
	char	*name;
	int	mode;
};

ulong ctlmode = 0664;

/*
 * Partition management (adapted from disk/partfs)
 */

Part *
lookpart(Umsc *lun, char *name)
{
	Part *part, *p;
	
	part = lun->part;
	for(p=part; p < &part[Qmax]; p++){
		if(p->inuse && strcmp(p->name, name) == 0)
			return p;
	}
	return nil;
}

Part *
freepart(Umsc *lun)
{
	Part *part, *p;
	
	part = lun->part;
	for(p=part; p < &part[Qmax]; p++){
		if(!p->inuse)
			return p;
	}
	return nil;
}

int
addpart(Umsc *lun, char *name, vlong start, vlong end, ulong mode)
{
	Part *p;

	if(start < 0 || start > end || end > lun->blocks){
		werrstr("bad partition boundaries");
		return -1;
	}
	p = lookpart(lun, name);
	if(p != nil){
		/* adding identical partition is no-op */
		if(p->offset == start && p->length == end - start && p->mode == mode)
			return 0;
		werrstr("partition name already in use");
		return -1;
	}
	p = freepart(lun);
	if(p == nil){
		werrstr("no free partition slots");
		return -1;
	}
	p->inuse = 1;
	free(p->name);
	p->id = p - lun->part;
	p->name = estrdup(name);
	p->offset = start;
	p->length = end - start;
	p->mode = mode;
	return 0;
}

int
delpart(Umsc *lun, char *s)
{
	Part *p;

	p = lookpart(lun, s);
	if(p == nil || p->id <= Qdata){
		werrstr("partition not found");
		return -1;
	}
	p->inuse = 0;
	free(p->name);
	p->name = nil;
	p->vers++;
	return 0;
}

void
fixlength(Umsc *lun, vlong blocks)
{
	Part *part, *p;
	
	part = lun->part;
	part[Qdata].length = blocks;
	for(p=&part[Qdata+1]; p < &part[Qmax]; p++){
		if(p->inuse && p->offset + p->length > blocks){
			if(p->offset > blocks){
				p->offset =blocks;
				p->length = 0;
			}else
				p->length = blocks - p->offset;
		}
	}
}

void
makeparts(Umsc *lun)
{
	addpart(lun, "/", 0, 0, DMDIR | 0555);
	addpart(lun, "ctl", 0, 0, 0664);
	addpart(lun, "raw", 0, 0, 0640);
	addpart(lun, "data", 0, lun->blocks, 0640);
}

/*
 * ctl parsing & formatting (adapted from partfs)
 */

static char*
ctlstring(Umsc *lun)
{
	Part *p, *part;
	Fmt fmt;
	
	part = &lun->part[0];

	fmtstrinit(&fmt);
	fmtprint(&fmt, "dev %s\n", dev->dir);
	fmtprint(&fmt, "lun %zd\n", lun - &ums->lun[0]);
	if(lun->flags & Finqok)
		fmtprint(&fmt, "inquiry %s\n", lun->inq);
	if(lun->blocks > 0)
		fmtprint(&fmt, "geometry %llud %ld\n", lun->blocks, lun->lbsize);
	for (p = &part[Qdata+1]; p < &part[Qmax]; p++)
		if (p->inuse)
			fmtprint(&fmt, "part %s %lld %lld\n",
				p->name, p->offset, p->offset + p->length);
	return fmtstrflush(&fmt);
}

static int
ctlparse(Umsc *lun, char *msg)
{
	vlong start, end;
	char *argv[16];
	int argc;
	
	argc = tokenize(msg, argv, nelem(argv));

	if(argc < 1){
		werrstr("empty control message");
		return -1;
	}

	if(strcmp(argv[0], "part") == 0){
		if(argc != 4){
			werrstr("part takes 3 args");
			return -1;
		}
		start = strtoll(argv[2], 0, 0);
		end = strtoll(argv[3], 0, 0);
		return addpart(lun, argv[1], start, end, 0640);
	}else if(strcmp(argv[0], "delpart") == 0){
		if(argc != 2){
			werrstr("delpart takes 1 arg");
			return -1;
		}
		return delpart(lun, argv[1]);
	}
	werrstr("unknown ctl");
	return -1;
}

/*
 * These are used by scuzz scsireq
 */
int exabyte, force6bytecmds;

int diskdebug;

static int
getmaxlun(void)
{
	uchar max;
	int r;

	max = 0;
	r = Rd2h|Rclass|Riface;
	if(usbcmd(dev, r, Getmaxlun, 0, 0, &max, 1) < 0){
		dprint(2, "disk: %s: getmaxlun failed: %r\n", dev->dir);
	}else{
		max &= 017;			/* 15 is the max. allowed */
		dprint(2, "disk: %s: maxlun %d\n", dev->dir, max);
	}
	return max;
}

static int
umsreset(void)
{
	int r;

	r = Rh2d|Rclass|Riface;
	if(usbcmd(dev, r, Umsreset, 0, 0, nil, 0) < 0){
		fprint(2, "disk: reset: %r\n");
		return -1;
	}
	sleep(100);
	return 0;
}

static int
umsrecover(void)
{
	if(umsreset() < 0)
		return -1;
	if(unstall(dev, ums->epin, Ein) < 0)
		dprint(2, "disk: unstall epin: %r\n");
	/* do we need this when epin == epout? */
	if(unstall(dev, ums->epout, Eout) < 0)
		dprint(2, "disk: unstall epout: %r\n");
	return 0;
}

static int
ispow2(uvlong ul)
{
	return (ul & (ul - 1)) == 0;
}

/*
 * return smallest power of 2 >= n
 */
static int
log2(int n)
{
	int i;

	for(i = 0; (1 << i) < n; i++)
		;
	return i;
}

static int
umsready(Umsc *lun)
{
	int i;

	for(i=0; i<3; i++){
		if(SRready(lun) != -1)
			break;

		if(lun->status != Status_SD)
			continue;

		/*
		 * logical unit is in process of becoming ready
		 * or initializing command required
		 */
		if(lun->sense[12] == 0x04)
		if(lun->sense[13] == 0x02 || lun->sense[13] == 0x01)
			break;

		/* medium not present */
		if(lun->sense[12] == 0x3A)
			return -1;
	}

	/* start unit */
	if((lun->inquiry[0] & 0x1F) == 0){
		SRstart(lun, 1);
		sleep(250);
	}

	return 0;
}

static int
umscapacity(Umsc *lun)
{
	uchar data[32];

	lun->blocks = 0;
	lun->capacity = 0;
	lun->lbsize = 0;
	memset(data, 0, sizeof data);

	if(umsready(lun) < 0)
		return -1;

	if(SRrcapacity(lun, data) < 0)
		return -1;

	lun->blocks = GETBELONG(data);
	lun->lbsize = GETBELONG(data+4);
	if(lun->blocks == 0xFFFFFFFF){
		if(SRrcapacity16(lun, data) < 0){
			lun->lbsize = 0;
			lun->blocks = 0;
			return -1;
		}else{
			lun->lbsize = GETBELONG(data + 8);
			lun->blocks = (uvlong)GETBELONG(data)<<32 |
				GETBELONG(data + 4);
		}
	}
	lun->blocks++; /* SRcapacity returns LBA of last block */
	lun->capacity = (vlong)lun->blocks * lun->lbsize;
	fixlength(lun, lun->blocks);
	if(diskdebug)
		fprint(2, "disk: logical block size %lud, # blocks %llud\n",
			lun->lbsize, lun->blocks);
	return 0;
}

static int
umsinit(void)
{
	uchar i;
	Umsc *lun;
	int some;

	umsreset();
	ums->maxlun = getmaxlun();
	ums->lun = mallocz((ums->maxlun+1) * sizeof(*ums->lun), 1);
	some = 0;
	for(i = 0; i <= ums->maxlun; i++){
		lun = &ums->lun[i];
		lun->ums = ums;
		lun->umsc = lun;
		lun->lun = i;
		lun->flags = Fopen | Fusb | Frw10;
		if(SRinquiry(lun) < 0 && SRinquiry(lun) < 0){
			dprint(2, "disk: lun %d inquiry failed\n", i);
			continue;
		}
		switch(lun->inquiry[0]){
		case Devdir:
		case Devworm:		/* a little different than the others */
		case Devcd:
		case Devmo:
			break;
		default:
			fprint(2, "disk: lun %d is not a disk (type %#02x)\n",
				i, lun->inquiry[0]);
			continue;
		}

		/*
		 * we ignore the device type reported by inquiry.
		 * Some devices return a wrong value but would still work.
		 */
		some++;
		lun->inq = smprint("%.48s", (char *)lun->inquiry+8);
		umscapacity(lun);
	}
	if(some == 0){
		dprint(2, "disk: all luns failed\n");
		devctl(dev, "detach");
		return -1;
	}
	return 0;
}


/*
 * called by SR*() commands provided by scuzz's scsireq
 */
long
umsrequest(Umsc *umsc, ScsiPtr *cmd, ScsiPtr *data, int *status)
{
	Cbw cbw;
	Csw csw;
	int n, nio;
	Ums *ums;

	ums = umsc->ums;

	memcpy(cbw.signature, "USBC", 4);
	cbw.tag = ++ums->seq;
	cbw.datalen = data->count;
	cbw.flags = data->write? CbwDataOut: CbwDataIn;
	cbw.lun = umsc->lun;
	if(cmd->count < 1 || cmd->count > 16)
		print("disk: umsrequest: bad cmd count: %ld\n", cmd->count);

	cbw.len = cmd->count;
	assert(cmd->count <= sizeof(cbw.command));
	memcpy(cbw.command, cmd->p, cmd->count);
	memset(cbw.command + cmd->count, 0, sizeof(cbw.command) - cmd->count);

	werrstr("");		/* we use %r later even for n == 0 */
	if(diskdebug){
		fprint(2, "disk: cmd: tag %#lx: ", cbw.tag);
		for(n = 0; n < cbw.len; n++)
			fprint(2, " %2.2x", cbw.command[n]&0xFF);
		fprint(2, " datalen: %ld\n", cbw.datalen);
	}

	/* issue tunnelled scsi command */
	if(write(ums->epout->dfd, &cbw, CbwLen) != CbwLen){
		fprint(2, "disk: cmd: %r\n");
		goto Fail;
	}

	/* transfer the data */
	nio = data->count;
	if(nio != 0){
		if(data->write)
			n = write(ums->epout->dfd, data->p, nio);
		else{
			n = read(ums->epin->dfd, data->p, nio);
			if (n >= 0 && n < nio)	/* didn't fill data->p? */
				memset(data->p + n, 0, nio - n);
		}
		if(diskdebug)
			if(n < 0)
				fprint(2, "disk: data: %r\n");
			else
				fprint(2, "disk: data: %d bytes (nio: %d)\n", n, nio);
		nio = n;
		if(n <= 0){
			nio = 0;
			if(data->write == 0)
				unstall(dev, ums->epin, Ein);
		}
	}

	/* read the transfer's status */
	n = read(ums->epin->dfd, &csw, CswLen);
	if(n <= 0){
		/* n == 0 means "stalled" */
		unstall(dev, ums->epin, Ein);
		n = read(ums->epin->dfd, &csw, CswLen);
	}

	if(n != CswLen || strncmp(csw.signature, "USBS", 4) != 0){
		dprint(2, "disk: read n=%d: status: %r\n", n);
		goto Fail;
	}
	if(csw.tag != cbw.tag){
		dprint(2, "disk: status tag mismatch\n");
		goto Fail;
	}
	if(csw.status >= CswPhaseErr){
		dprint(2, "disk: phase error\n");
		goto Fail;
	}
	if(csw.dataresidue == 0 || ums->wrongresidues)
		csw.dataresidue = data->count - nio;
	if(diskdebug){
		fprint(2, "disk: status: %2.2ux residue: %ld\n",
			csw.status, csw.dataresidue);
		if(cbw.command[0] == ScmdRsense){
			fprint(2, "sense data:");
			for(n = 0; n < data->count - csw.dataresidue; n++)
				fprint(2, " %2.2x", data->p[n]);
			fprint(2, "\n");
		}
	}
	switch(csw.status){
	case CswOk:
		*status = STok;
		break;
	case CswFailed:
		*status = STcheck;
		break;
	default:
		dprint(2, "disk: phase error\n");
		goto Fail;
	}
	ums->nerrs = 0;
	return data->count - csw.dataresidue;

Fail:
	*status = STharderr;
	if(ums->nerrs++ > 15)
		sysfatal("%s: too many errors", dev->dir);
	umsrecover();
	return -1;
}

static void
dattach(Req *req)
{
	req->fid->qid = (Qid) {0, 0, QTDIR};
	req->ofcall.qid = req->fid->qid;
	respond(req, nil);
}

static char *
dwalk(Fid *fid, char *name, Qid *qid)
{
	Umsc *lun;
	Part *p;

	if((fid->qid.type & QTDIR) == 0)
		return "walk in non-directory";
	if(strcmp(name, "..") == 0){
		fid->qid = (Qid){0, 0, QTDIR};
		*qid = fid->qid;
		return nil;
	}
	if(fid->qid.path == 0){
		for(lun = ums->lun; lun <= ums->lun + ums->maxlun; lun++)
			if(strcmp(lun->name, name) == 0){
				fid->qid.path = (lun - ums->lun + 1) << 16;
				fid->qid.vers = 0;
				fid->qid.type = QTDIR;
				*qid = fid->qid;
				return nil;
			}
		return "does not exist";
	}
	lun = ums->lun + (fid->qid.path >> 16) - 1;
	p = lookpart(lun, name);
	if(p == nil)
		return "does not exist";
	fid->qid.path |= p->id;
	fid->qid.vers = p->vers;
	fid->qid.type = p->mode >> 24;
	*qid = fid->qid;
	return nil;
}

static void
dostat(Umsc *lun, int path, Dir *d)
{
	Part *p;

	p = &lun->part[path];
	d->qid.path = path;
	d->qid.vers = p->vers;
	d->qid.type =p->mode >> 24;
	d->mode = p->mode;
	d->length = (vlong) p->length * lun->lbsize;
	d->name = strdup(p->name);
	d->uid = strdup(getuser());
	d->gid = strdup(d->uid);
	d->muid = strdup(d->uid);
}

static int
dirgen(int n, Dir* d, void *aux)
{
	Umsc *lun;
	int i;
	
	lun = aux;
	for(i = Qctl; i < Qmax; i++){
		if(lun->part[i].inuse == 0)
			continue;
		if(n-- == 0)
			break;
	}
	if(i == Qmax)
		return -1;
	dostat(lun, i, d);
	d->qid.path |= (lun - ums->lun) << 16;
	return 0;
}

static int
rootgen(int n, Dir *d, void *)
{
	Umsc *lun;

	if(n > ums->maxlun)
		return -1;
	lun = ums->lun + n;
	d->qid.path = (n + 1) << 16;
	d->qid.type = QTDIR;
	d->mode = 0555 | DMDIR;
	d->name = strdup(lun->name);
	d->uid = strdup(getuser());
	d->gid = strdup(d->uid);
	d->muid = strdup(d->uid);
	return 0;
}


static void
dstat(Req *req)
{
	int path;
	Dir *d;
	Umsc *lun;

	d = &req->d;
	d->qid = req->fid->qid;
	if(req->fid->qid.path == 0){
		d->name = strdup("");
		d->mode = 0555 | DMDIR;
		d->uid = strdup(getuser());
		d->gid = strdup(d->uid);
		d->muid = strdup(d->uid);
	}else{
		path = req->fid->qid.path & 0xFFFF;
		lun = ums->lun + (req->fid->qid.path >> 16) - 1;
		dostat(lun, path, d);
	}
	respond(req, nil);
}

static void
dopen(Req *req)
{
	ulong path;
	Umsc *lun;

	if(req->ofcall.qid.path == 0){
		respond(req, nil);
		return;
	}
	path = req->ofcall.qid.path & 0xFFFF;
	lun = ums->lun + (req->ofcall.qid.path >> 16) - 1;
	switch(path){
	case Qraw:
		srvrelease(req->srv);
		qlock(lun);
		lun->phase = Pcmd;
		qunlock(lun);
		srvacquire(req->srv);
		break;
	}
	respond(req, nil);
}

/*
 * check i/o parameters and compute values needed later.
 * we shift & mask manually to avoid run-time calls to _divv and _modv,
 * since we don't need general division nor its cost.
 */
static int
setup(Umsc *lun, Part *p, char *data, int count, vlong offset)
{
	long nb, lbsize, lbshift, lbmask;
	uvlong bno;

	if(count < 0 || lun->lbsize <= 0 && umscapacity(lun) < 0 ||
	    lun->lbsize == 0)
		return -1;
	lbsize = lun->lbsize;
	assert(ispow2(lbsize));
	lbshift = log2(lbsize);
	lbmask = lbsize - 1;

	bno = offset >> lbshift;	/* offset / lbsize */
	nb = ((offset + count + lbsize - 1) >> lbshift) - bno;

	if(bno + nb > p->length)		/* past end of partition? */
		nb = p->length - bno;
	if(nb * lbsize > Maxiosize)
		nb = Maxiosize / lbsize;
	lun->nb = nb;
	if(bno >= p->length || nb == 0)
		return 0;

	bno += p->offset;		/* start of partition */
	lun->offset = bno;
	lun->off = offset & lbmask;		/* offset % lbsize */
	if(lun->off == 0 && (count & lbmask) == 0)
		lun->bufp = data;
	else
		/* not transferring full, aligned blocks; need intermediary */
		lun->bufp = lun->buf;
	return count;
}

/*
 * Upon SRread/SRwrite errors we assume the medium may have changed,
 * and ask again for the capacity of the media.
 * BUG: How to proceed to avoid confussing dossrv??
 */
static void
dread(Req *req)
{
	long n;
	ulong path;
	char buf[64];
	char *s;
	Part *p;
	Umsc *lun;
	Qid q;
	long count;
	void *data;
	vlong offset;
	Srv *srv;

	q = req->fid->qid;
	if(q.path == 0){
		dirread9p(req, rootgen, nil);
		respond(req, nil);
		return;
	}
	path = q.path & 0xFFFF;
	lun = ums->lun + (q.path >> 16) - 1;
	count = req->ifcall.count;
	data = req->ofcall.data;
	offset = req->ifcall.offset;

	switch(path){
	case Qdir:
		dirread9p(req, dirgen, lun);
		respond(req, nil);
		return;
	case Qctl:
		s = ctlstring(lun);
		readstr(req, s);
		free(s);
		respond(req, nil);
		return;
	}

	srv = req->srv;
	srvrelease(srv);
	qlock(lun);
	switch(path){
	case Qraw:
		if(lun->lbsize <= 0 && umscapacity(lun) < 0){
			respond(req, "phase error");
			break;
		}
		switch(lun->phase){
		case Pcmd:
			respond(req, "phase error");
			break;
		case Pdata:
			lun->data.p = data;
			lun->data.count = count;
			lun->data.write = 0;
			count = umsrequest(lun,&lun->cmd,&lun->data,&lun->status);
			lun->phase = Pstatus;
			if(count < 0){
				lun->lbsize = 0;  /* medium may have changed */
				responderror(req);
			}else{
				req->ofcall.count = count;
				respond(req, nil);
			}
			break;
		case Pstatus:
			n = snprint(buf, sizeof buf, "%11.0ud ", lun->status);
			readbuf(req, buf, n);
			lun->phase = Pcmd;
			respond(req, nil);
			break;
		}
		break;
	case Qdata:
	default:
		p = &lun->part[path];
		if(!p->inuse){
			respond(req, "permission denied");
			break;
		}
		count = setup(lun, p, data, count, offset);
		if (count <= 0){
			responderror(req);
			break;
		}
		n = SRread(lun, lun->bufp, lun->nb * lun->lbsize);
		if(n < 0){
			lun->lbsize = 0;	/* medium may have changed */
			responderror(req);
			break;
		} else if (lun->bufp == data)
			count = n;
		else{
			/*
			 * if n == lun->nb*lun->lbsize (as expected),
			 * just copy count bytes.
			 */
			if(lun->off + count > n)
				count = n - lun->off; /* short read */
			if(count > 0)
				memmove(data, lun->bufp + lun->off, count);
		}
		req->ofcall.count = count;
		respond(req, nil);
		break;
	}

	qunlock(lun);
	srvacquire(srv);
}

static void
dwrite(Req *req)
{
	long len, ocount;
	ulong path;
	uvlong bno;
	Part *p;
	Umsc *lun;
	char *s;
	long count;
	void *data;
	vlong offset;
	Srv *srv;

	lun = ums->lun + (req->fid->qid.path >> 16) - 1;
	path = req->fid->qid.path & 0xFFFF;
	count = req->ifcall.count;
	data = req->ifcall.data;
	offset = req->ifcall.offset;

	srv = req->srv;
	srvrelease(srv);
	qlock(lun);

	switch(path){
	case Qctl:
		s = emallocz(count+1, 1);
		memmove(s, data, count);
		if(s[count-1] == '\n')
			s[count-1] = 0;
		if(ctlparse(lun, s) == -1)
			responderror(req);
		else{
			req->ofcall.count = count;
			respond(req, nil);
		}
		free(s);
		break;
	case Qraw:
		if(lun->lbsize <= 0 && umscapacity(lun) < 0){
			respond(req, "phase error");
			break;
		}
		switch(lun->phase){
		case Pcmd:
			if(count != 6 && count != 10 && count != 12 && count != 16){
				respond(req, "bad command length");
				break;
			}
			memmove(lun->rawcmd, data, count);
			lun->cmd.p = lun->rawcmd;
			lun->cmd.count = count;
			lun->cmd.write = 1;
			lun->phase = Pdata;
			req->ofcall.count = count;
			respond(req, nil);
			break;
		case Pdata:
			lun->data.p = data;
			lun->data.count = count;
			lun->data.write = 1;
			count = umsrequest(lun,&lun->cmd,&lun->data,&lun->status);
			lun->phase = Pstatus;
			if(count < 0){
				lun->lbsize = 0;  /* medium may have changed */
				responderror(req);
			}else{
				req->ofcall.count = count;
				respond(req, nil);
			}
			break;
		case Pstatus:
			lun->phase = Pcmd;
			respond(req, "phase error");
			break;
		}
		break;
	case Qdata:
	default:
		p = &lun->part[path];
		if(!p->inuse){
			respond(req, "permission denied");
			break;
		}
		len = ocount = count;
		count = setup(lun, p, data, count, offset);
		if (count <= 0){
			responderror(req);
			break;
		}
		bno = lun->offset;
		if (lun->bufp == lun->buf) {
			count = SRread(lun, lun->bufp, lun->nb * lun->lbsize);
			if(count < 0) {
				lun->lbsize = 0;  /* medium may have changed */
				responderror(req);
				break;
			}
			/*
			 * if count == lun->nb*lun->lbsize, as expected, just
			 * copy len (the original count) bytes of user data.
			 */
			if(lun->off + len > count)
				len = count - lun->off; /* short read */
			if(len > 0)
				memmove(lun->bufp + lun->off, data, len);
		}

		lun->offset = bno;
		count = SRwrite(lun, lun->bufp, lun->nb * lun->lbsize);
		if(count < 0){
			lun->lbsize = 0;	/* medium may have changed */
			responderror(req);
			break;
		}else{
			if(lun->off + len > count)
				count -= lun->off; /* short write */
			/* never report more bytes written than requested */
			if(count < 0)
				count = 0;
			else if(count > ocount)
				count = ocount;
		}
		req->ofcall.count = count;
		respond(req, nil);
		break;
	}

	qunlock(lun);
	srvacquire(srv);
}

int
findendpoints(Ums *ums)
{
	Ep *ep;
	Usbdev *ud;
	ulong csp, sc;
	int i, epin, epout;

	epin = epout = -1;
	ud = dev->usb;
	for(i = 0; i < nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			continue;
		csp = ep->iface->csp;
		sc = Subclass(csp);
		if(!(Class(csp) == Clstorage && (Proto(csp) == Protobulk || Proto(csp) == Protouas)))
			continue;
		if(sc != Subatapi && sc != Sub8070 && sc != Subscsi)
			fprint(2, "disk: subclass %#ulx not supported. trying anyway\n", sc);
		if(ep->type == Ebulk){
			if(ep->dir == Eboth || ep->dir == Ein)
				if(epin == -1)
					epin =  ep->id;
			if(ep->dir == Eboth || ep->dir == Eout)
				if(epout == -1)
					epout = ep->id;
		}
	}
	dprint(2, "disk: ep ids: in %d out %d\n", epin, epout);
	if(epin == -1 || epout == -1)
		return -1;
	ums->epin = openep(dev, epin);
	if(ums->epin == nil){
		fprint(2, "disk: openep %d: %r\n", epin);
		return -1;
	}
	if(epout == epin){
		incref(ums->epin);
		ums->epout = ums->epin;
	}else
		ums->epout = openep(dev, epout);
	if(ums->epout == nil){
		fprint(2, "disk: openep %d: %r\n", epout);
		closedev(ums->epin);
		return -1;
	}
	if(ums->epin == ums->epout)
		opendevdata(ums->epin, ORDWR);
	else{
		opendevdata(ums->epin, OREAD);
		opendevdata(ums->epout, OWRITE);
	}
	if(ums->epin->dfd < 0 || ums->epout->dfd < 0){
		fprint(2, "disk: open i/o ep data: %r\n");
		closedev(ums->epin);
		closedev(ums->epout);
		return -1;
	}
	dprint(2, "disk: ep in %s out %s\n", ums->epin->dir, ums->epout->dir);

	devctl(ums->epin, "timeout 2000");
	devctl(ums->epout, "timeout 2000");

	if(usbdebug > 1 || diskdebug > 2){
		devctl(ums->epin, "debug 1");
		devctl(ums->epout, "debug 1");
		devctl(dev, "debug 1");
	}
	return 0;
}

static void
usage(void)
{
	fprint(2, "usage: %s [-d] devid\n", argv0);
	exits("usage");
}

/*
 * some devices like usb modems appear as mass storage
 * for windows driver installation. switch mode here
 * and exit if we detect one so the real driver can
 * attach it.
 */ 
static void
notreallyums(Dev *dev)
{
	/* HUAWEI E220 */
	if(dev->usb->vid == 0x12d1 && dev->usb->did == 0x1003){
		usbcmd(dev, Rh2d|Rstd|Rdev, Rsetfeature, Fdevremotewakeup, 0x02, nil, 0);
		exits("mode switch");
	}
}

static Srv diskfs = {
	.attach = dattach,
	.walk1 = dwalk,
	.open =	 dopen,
	.read =	 dread,
	.write = dwrite,
	.stat =	 dstat,
};

void
main(int argc, char **argv)
{
	Umsc *lun;
	int i;
	char buf[20];

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		scsidebug(diskdebug);
		diskdebug++;
		break;
	default:
		usage();
	}ARGEND
	if(argc != 1)
		usage();
	dev = getdev(*argv);
	if(dev == nil)
		sysfatal("getdev: %r");
	notreallyums(dev);
	ums = dev->aux = emallocz(sizeof(Ums), 1);
	ums->maxlun = -1;
	if(findendpoints(ums) < 0)
		sysfatal("endpoints not found");

	/*
	 * SanDISK 512M gets residues wrong.
	 */
	if(dev->usb->vid == 0x0781 && dev->usb->did == 0x5150)
		ums->wrongresidues = 1;

	if(umsinit() < 0)
		sysfatal("umsinit: %r\n");

	for(i = 0; i <= ums->maxlun; i++){
		lun = &ums->lun[i];
		if(ums->maxlun > 0)
			snprint(lun->name, sizeof(lun->name), "sdU%s.%d", dev->hname, i);
		else
			snprint(lun->name, sizeof(lun->name), "sdU%s", dev->hname);
		makeparts(lun);
	}
	snprint(buf, sizeof buf, "%d.disk", dev->id);
	postsharesrv(&diskfs, nil, "usb", buf);
	exits(nil);
}
