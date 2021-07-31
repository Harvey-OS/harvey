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
#include "scsireq.h"
#include "usb.h"
#include "usbfs.h"
#include "ums.h"

enum
{
	Qdir = 0,
	Qctl,
	Qraw,
	Qdata,
	Qmax,
};

typedef struct Dirtab Dirtab;
struct Dirtab
{
	char	*name;
	int	mode;
};

static Dirtab dirtab[] =
{
	[Qdir]	"/",	DMDIR|0555,
	[Qctl]	"ctl",	0664,		/* nothing secret here */
	[Qraw]	"raw",	0640,
	[Qdata]	"data",	0640,
};

/*
 * These are used by scuzz scsireq
 */
int exabyte, force6bytecmds;

int diskdebug;

static void
ding(void *, char *msg)
{
	if(strstr(msg, "alarm") != nil)
		noted(NCONT);
	noted(NDFLT);
}

static int
getmaxlun(Dev *dev)
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
umsreset(Ums *ums)
{
	int r;

	r = Rh2d|Rclass|Riface;
	if(usbcmd(ums->dev, r, Umsreset, 0, 0, nil, 0) < 0){
		fprint(2, "disk: reset: %r\n");
		return -1;
	}
	return 0;
}

static int
umsrecover(Ums *ums)
{
	if(umsreset(ums) < 0)
		return -1;
	if(unstall(ums->dev, ums->epin, Ein) < 0)
		dprint(2, "disk: unstall epin: %r\n");

	/* do we need this when epin == epout? */
	if(unstall(ums->dev, ums->epout, Eout) < 0)
		dprint(2, "disk: unstall epout: %r\n");
	return 0;
}

static void
umsfatal(Ums *ums)
{
	int i;

	devctl(ums->dev, "detach");
	for(i = 0; i < ums->maxlun; i++)
		usbfsdel(&ums->lun[i].fs);
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
umscapacity(Umsc *lun)
{
	uchar data[32];

	lun->blocks = 0;
	lun->capacity = 0;
	lun->lbsize = 0;
	memset(data, 0, sizeof data);
	if(SRrcapacity(lun, data) < 0 && SRrcapacity(lun, data)  < 0)
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
	if(diskdebug)
		fprint(2, "disk: logical block size %lud, # blocks %llud\n",
			lun->lbsize, lun->blocks);
	return 0;
}

static int
umsinit(Ums *ums)
{
	uchar i;
	Umsc *lun;
	int some;

	umsreset(ums);
	ums->maxlun = getmaxlun(ums->dev);
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
		SRstart(lun, 1);
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
		devctl(ums->dev, "detach");
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
	int n, nio, left;
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
			left = nio - n;
			if (n >= 0 && left > 0)	/* didn't fill data->p? */
				memset(data->p + n, 0, left);
		}
		nio = n;
		if(diskdebug)
			if(n < 0)
				fprint(2, "disk: data: %r\n");
			else
				fprint(2, "disk: data: %d bytes\n", n);
		if(n <= 0)
			if(data->write == 0)
				unstall(ums->dev, ums->epin, Ein);
	}

	/* read the transfer's status */
	n = read(ums->epin->dfd, &csw, CswLen);
	if(n <= 0){
		/* n == 0 means "stalled" */
		unstall(ums->dev, ums->epin, Ein);
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
	if(ums->nerrs++ > 15){
		fprint(2, "disk: %s: too many errors: device detached\n", ums->dev->dir);
		umsfatal(ums);
	}else
		umsrecover(ums);
	return -1;
}

static int
dwalk(Usbfs *fs, Fid *fid, char *name)
{
	int i;
	Qid qid;

	qid = fid->qid;
	if((qid.type & QTDIR) == 0){
		werrstr("walk in non-directory");
		return -1;
	}

	if(strcmp(name, "..") == 0)
		return 0;

	for(i = 1; i < nelem(dirtab); i++)
		if(strcmp(name, dirtab[i].name) == 0){
			qid.path = i | fs->qid;
			qid.vers = 0;
			qid.type = dirtab[i].mode >> 24;
			fid->qid = qid;
			return 0;
		}
	werrstr(Enotfound);
	return -1;
}

static void
dostat(Usbfs *fs, int path, Dir *d)
{
	Dirtab *t;
	Umsc *lun;

	t = &dirtab[path];
	d->qid.path = path;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
	d->name = t->name;
	lun = fs->aux;
	if(path == Qdata)
		d->length = lun->capacity;
	else
		d->length = 0;
}

static int
dirgen(Usbfs *fs, Qid, int i, Dir *d, void*)
{
	i++;	/* skip dir */
	if(i >= Qmax)
		return -1;
	else{
		dostat(fs, i, d);
		d->qid.path |= fs->qid;
		return 0;
	}
}

static int
dstat(Usbfs *fs, Qid qid, Dir *d)
{
	int path;

	path = qid.path & ~fs->qid;
	dostat(fs, path, d);
	d->qid.path |= fs->qid;
	return 0;
}

static int
dopen(Usbfs *fs, Fid *fid, int)
{
	ulong path;
	Umsc *lun;

	path = fid->qid.path & ~fs->qid;
	lun = fs->aux;
	switch(path){
	case Qraw:
		lun->phase = Pcmd;
		break;
	}
	return 0;
}

/*
 * check i/o parameters and compute values needed later.
 * we shift & mask manually to avoid run-time calls to _divv and _modv,
 * since we don't need general division nor its cost.
 */
static int
setup(Umsc *lun, char *data, int count, vlong offset)
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

	if(bno + nb > lun->blocks)		/* past end of device? */
		nb = lun->blocks - bno;
	if(nb * lbsize > Maxiosize)
		nb = Maxiosize / lbsize;
	lun->nb = nb;
	if(bno >= lun->blocks || nb == 0)
		return 0;

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
static long
dread(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	long n;
	ulong path;
	char buf[1024];
	char *s, *e;
	Umsc *lun;
	Ums *ums;
	Qid q;

	q = fid->qid;
	path = fid->qid.path & ~fs->qid;
	ums = fs->dev->aux;
	lun = fs->aux;

	qlock(ums);
	switch(path){
	case Qdir:
		count = usbdirread(fs, q, data, count, offset, dirgen, nil);
		break;
	case Qctl:
		e = buf + sizeof(buf);
		s = seprint(buf, e, "%s lun %ld: ", fs->dev->dir, lun - &ums->lun[0]);
		if(lun->flags & Finqok)
			s = seprint(s, e, "inquiry %s ", lun->inq);
		if(lun->blocks > 0)
			s = seprint(s, e, "geometry %llud %ld",
				lun->blocks, lun->lbsize);
		s = seprint(s, e, "\n");
		count = usbreadbuf(data, count, offset, buf, s - buf);
		break;
	case Qraw:
		if(lun->lbsize <= 0 && umscapacity(lun) < 0){
			count = -1;
			break;
		}
		switch(lun->phase){
		case Pcmd:
			qunlock(ums);
			werrstr("phase error");
			return -1;
		case Pdata:
			lun->data.p = data;
			lun->data.count = count;
			lun->data.write = 0;
			count = umsrequest(lun,&lun->cmd,&lun->data,&lun->status);
			lun->phase = Pstatus;
			if(count < 0)
				lun->lbsize = 0;  /* medium may have changed */
			break;
		case Pstatus:
			n = snprint(buf, sizeof buf, "%11.0ud ", lun->status);
			count = usbreadbuf(data, count, 0LL, buf, n);
			lun->phase = Pcmd;
			break;
		}
		break;
	case Qdata:
		count = setup(lun, data, count, offset);
		if (count <= 0)
			break;
		n = SRread(lun, lun->bufp, lun->nb * lun->lbsize);
		if(n < 0){
			lun->lbsize = 0;	/* medium may have changed */
			count = -1;
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
		break;
	}
	qunlock(ums);
	return count;
}

static long
dwrite(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	long len, ocount;
	ulong path;
	uvlong bno;
	Ums *ums;
	Umsc *lun;

	ums = fs->dev->aux;
	lun = fs->aux;
	path = fid->qid.path & ~fs->qid;

	qlock(ums);
	switch(path){
	default:
		werrstr(Eperm);
		count = -1;
		break;
	case Qctl:
		dprint(2, "usb/disk: ctl ignored\n");
		break;
	case Qraw:
		if(lun->lbsize <= 0 && umscapacity(lun) < 0){
			count = -1;
			break;
		}
		switch(lun->phase){
		case Pcmd:
			if(count != 6 && count != 10){
				qunlock(ums);
				werrstr("bad command length");
				return -1;
			}
			memmove(lun->rawcmd, data, count);
			lun->cmd.p = lun->rawcmd;
			lun->cmd.count = count;
			lun->cmd.write = 1;
			lun->phase = Pdata;
			break;
		case Pdata:
			lun->data.p = data;
			lun->data.count = count;
			lun->data.write = 1;
			count = umsrequest(lun,&lun->cmd,&lun->data,&lun->status);
			lun->phase = Pstatus;
			if(count < 0)
				lun->lbsize = 0;  /* medium may have changed */
			break;
		case Pstatus:
			lun->phase = Pcmd;
			werrstr("phase error");
			count = -1;
			break;
		}
		break;
	case Qdata:
		len = ocount = count;
		count = setup(lun, data, count, offset);
		if (count <= 0)
			break;
		bno = lun->offset;
		if (lun->bufp == lun->buf) {
			count = SRread(lun, lun->bufp, lun->nb * lun->lbsize);
			if(count < 0) {
				lun->lbsize = 0;  /* medium may have changed */
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
		if(count < 0)
			lun->lbsize = 0;	/* medium may have changed */
		else{
			if(lun->off + len > count)
				count -= lun->off; /* short write */
			/* never report more bytes written than requested */
			if(count < 0)
				count = 0;
			else if(count > ocount)
				count = ocount;
		}
		break;
	}
	qunlock(ums);
	return count;
}

int
findendpoints(Ums *ums)
{
	Ep *ep;
	Usbdev *ud;
	ulong csp, sc;
	int i, epin, epout;

	epin = epout = -1;
	ud = ums->dev->usb;
	for(i = 0; i < nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			continue;
		csp = ep->iface->csp;
		sc = Subclass(csp);
		if(!(Class(csp) == Clstorage && (Proto(csp) == Protobulk)))
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
	ums->epin = openep(ums->dev, epin);
	if(ums->epin == nil){
		fprint(2, "disk: openep %d: %r\n", epin);
		return -1;
	}
	if(epout == epin){
		incref(ums->epin);
		ums->epout = ums->epin;
	}else
		ums->epout = openep(ums->dev, epout);
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
		devctl(ums->dev, "debug 1");
	}
	return 0;
}

static int
usage(void)
{
	werrstr("usage: usb/disk [-d] [-N nb]");
	return -1;
}

static void
umsdevfree(void *a)
{
	Ums *ums = a;

	if(ums == nil)
		return;
	closedev(ums->epin);
	closedev(ums->epout);
	ums->epin = ums->epout = nil;
	free(ums->lun);
	free(ums);
}

static Usbfs diskfs = {
	.walk = dwalk,
	.open =	 dopen,
	.read =	 dread,
	.write = dwrite,
	.stat =	 dstat,
};

int
diskmain(Dev *dev, int argc, char **argv)
{
	Ums *ums;
	Umsc *lun;
	int i, devid;

	devid = dev->id;
	ARGBEGIN{
	case 'd':
		scsidebug(diskdebug);
		diskdebug++;
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

//	notify(ding);
	ums = dev->aux = emallocz(sizeof(Ums), 1);
	ums->maxlun = -1;
	ums->dev = dev;
	dev->free = umsdevfree;
	if(findendpoints(ums) < 0){
		werrstr("disk: endpoints not found");
		return -1;
	}

	/*
	 * SanDISK 512M gets residues wrong.
	 */
	if(dev->usb->vid == 0x0781 && dev->usb->did == 0x5150)
		ums->wrongresidues = 1;

	if(umsinit(ums) < 0){
		dprint(2, "disk: umsinit: %r\n");
		return -1;
	}

	for(i = 0; i <= ums->maxlun; i++){
		lun = &ums->lun[i];
		lun->fs = diskfs;
		snprint(lun->fs.name, sizeof(lun->fs.name), "sdU%d.%d", devid, i);
		lun->fs.dev = dev;
		incref(dev);
		lun->fs.aux = lun;
		usbfsadd(&lun->fs);
	}
	return 0;
}
