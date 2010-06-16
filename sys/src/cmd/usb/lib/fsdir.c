#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "usb.h"
#include "usbfs.h"

typedef struct Rpc Rpc;

enum
{
	Incr = 3,	/* increments for fs array */
	Dtop = 0,	/* high 32 bits for / 	*/
	Qdir = 0,	/* low 32 bits for /devdir */
};

QLock fslck;
static Usbfs** fs;
static int nfs;
static int fsused;
static int exitonclose = 1;

void
usbfsexits(int y)
{
	exitonclose = y;
}

static int
qiddev(uvlong path)
{
	return (int)(path>>32) & 0xFF;
}

static int
qidfile(uvlong path)
{
	return (int)(path & 0xFFFFFFFFULL);
}

static uvlong
mkqid(int qd, int qf)
{
	return ((uvlong)qd << 32) | (uvlong)qf;
}

void
usbfsdirdump(void)
{
	int i;

	qlock(&fslck);
	fprint(2, "%s: fs list: (%d used %d total)\n", argv0, fsused, nfs);
	for(i = 1; i < nfs; i++)
		if(fs[i] != nil)
			if(fs[i]->dev != nil)
				fprint(2, "%s\t%s dev %#p refs %ld\n",
					argv0, fs[i]->name, fs[i]->dev, fs[i]->dev->ref);
			else
				fprint(2, "%s:\t%s\n", argv0, fs[i]->name);
	qunlock(&fslck);
}

void
usbfsadd(Usbfs *dfs)
{
	int i, j;

	dprint(2, "%s: fsadd %s\n", argv0, dfs->name);
	qlock(&fslck);
	for(i = 1; i < nfs; i++)
		if(fs[i] == nil)
			break;
	if(i >= nfs){
		if((nfs%Incr) == 0){
			fs = realloc(fs, sizeof(Usbfs*) * (nfs+Incr));
			if(fs == nil)
				sysfatal("realloc: %r");
			for(j = nfs; j < nfs+Incr; j++)
				fs[j] = nil;
		}
		if(nfs == 0)	/* do not use entry 0 */
			nfs++;
		fs[nfs++] = dfs;
	}else
		fs[i] = dfs;
	dfs->qid = mkqid(i, 0);
	fsused++;
	qunlock(&fslck);
}

static void
usbfsdelnth(int i)
{
	if(fs[i] != nil){
		dprint(2, "%s: fsdel %s", argv0, fs[i]->name);
		if(fs[i]->dev != nil){
			dprint(2, " dev %#p ref %ld\n",
				fs[i]->dev, fs[i]->dev->ref);
		}else
			dprint(2, "no dev\n");
		if(fs[i]->end != nil)
			fs[i]->end(fs[i]);
		closedev(fs[i]->dev);
		fsused--;
	}
	fs[i] = nil;
	if(fsused == 0 && exitonclose != 0){
		fprint(2, "%s: all file systems gone: exiting\n", argv0);
		threadexitsall(nil);
	}
}

void
usbfsdel(Usbfs *dfs)
{
	int i;

	qlock(&fslck);
	for(i = 0; i < nfs; i++)
		if(dfs == nil || fs[i] == dfs){
			usbfsdelnth(i);
			if(dfs != nil)
				break;
		}
	qunlock(&fslck);
}

static void
fsend(Usbfs*)
{
	dprint(2, "%s: fsend\n", argv0);
	usbfsdel(nil);
}

void
usbfsgone(char *dir)
{
	int i;

	qlock(&fslck);
	/* devices may have more than one fs */
	for(i = 0; i < nfs; i++)
		if(fs[i] != nil && fs[i]->dev != nil)
		if(strcmp(fs[i]->dev->dir, dir) == 0)
			usbfsdelnth(i);
	qunlock(&fslck);
}

static void
fsclone(Usbfs*, Fid *o, Fid *n)
{
	int qd;
	Dev *dev;
	void (*xfsclone)(Usbfs *fs, Fid *of, Fid *nf);

	xfsclone = nil;
	dev = nil;
	qd = qiddev(o->qid.path);
	qlock(&fslck);
	if(qd != Dtop && fs[qd] != nil && fs[qd]->clone != nil){
		dev = fs[qd]->dev;
		if(dev != nil)
			incref(dev);
		xfsclone = fs[qd]->clone;
	}
	qunlock(&fslck);
	if(xfsclone != nil){
		xfsclone(fs[qd], o, n);
	}
	if(dev != nil)
		closedev(dev);
}

static int
fswalk(Usbfs*, Fid *fid, char *name)
{
	Qid q;
	int qd, qf;
	int i;
	int rc;
	Dev *dev;
	Dir d;
	int (*xfswalk)(Usbfs *fs, Fid *f, char *name);

	q = fid->qid;
	qd = qiddev(q.path);
	qf = qidfile(q.path);

	q.type = QTDIR;
	q.vers = 0;
	if(strcmp(name, "..") == 0)
		if(qd == Dtop || qf == Qdir){
			q.path = mkqid(Dtop, Qdir);
			fid->qid = q;
			return 0;
		}
	if(qd != 0){
		qlock(&fslck);
		if(fs[qd] == nil){
			qunlock(&fslck);
			werrstr(Eio);
			return -1;
		}
		dev = fs[qd]->dev;
		if(dev != nil)
			incref(dev);
		xfswalk = fs[qd]->walk;
		qunlock(&fslck);
		rc = xfswalk(fs[qd], fid, name);
		if(dev != nil)
			closedev(dev);
		return rc;
	}
	qlock(&fslck);
	for(i = 0; i < nfs; i++)
		if(fs[i] != nil && strcmp(name, fs[i]->name) == 0){
			q.path = mkqid(i, Qdir);
			fs[i]->stat(fs[i], q, &d); /* may be a file */
			fid->qid = d.qid;
			qunlock(&fslck);
			return 0;
		}
	qunlock(&fslck);
	werrstr(Enotfound);
	return -1;
}

static int
fsopen(Usbfs*, Fid *fid, int mode)
{
	int qd;
	int rc;
	Dev *dev;
	int (*xfsopen)(Usbfs *fs, Fid *f, int mode);

	qd = qiddev(fid->qid.path);
	if(qd == Dtop)
		return 0;
	qlock(&fslck);
	if(fs[qd] == nil){
		qunlock(&fslck);
		werrstr(Eio);
		return -1;
	}
	dev = fs[qd]->dev;
	if(dev != nil)
		incref(dev);
	xfsopen = fs[qd]->open;
	qunlock(&fslck);
	if(xfsopen != nil)
		rc = xfsopen(fs[qd], fid, mode);
	else
		rc = 0;
	if(dev != nil)
		closedev(dev);
	return rc;
}

static int
dirgen(Usbfs*, Qid, int n, Dir *d, void *)
{
	int i;
	Dev *dev;
	char *nm;

	qlock(&fslck);
	for(i = 0; i < nfs; i++)
		if(fs[i] != nil && n-- == 0){
			d->qid.type = QTDIR;
			d->qid.path = mkqid(i, Qdir);
			d->qid.vers = 0;
			dev = fs[i]->dev;
			if(dev != nil)
				incref(dev);
			nm = d->name;
			fs[i]->stat(fs[i], d->qid, d);
			d->name = nm;
			strncpy(d->name, fs[i]->name, Namesz);
			if(dev != nil)
				closedev(dev);
			qunlock(&fslck);
			return 0;
		}
	qunlock(&fslck);
	return -1;
}

static long
fsread(Usbfs*, Fid *fid, void *data, long cnt, vlong off)
{
	int qd;
	int rc;
	Dev *dev;
	Qid q;
	long (*xfsread)(Usbfs *fs, Fid *f, void *data, long count, vlong );

	q = fid->qid;
	qd = qiddev(q.path);
	if(qd == Dtop)
		return usbdirread(nil, q, data, cnt, off, dirgen, nil);
	qlock(&fslck);
	if(fs[qd] == nil){
		qunlock(&fslck);
		werrstr(Eio);
		return -1;
	}
	dev = fs[qd]->dev;
	if(dev != nil)
		incref(dev);
	xfsread = fs[qd]->read;
	qunlock(&fslck);
	rc = xfsread(fs[qd], fid, data, cnt, off);
	if(dev != nil)
		closedev(dev);
	return rc;
}

static long
fswrite(Usbfs*, Fid *fid, void *data, long cnt, vlong off)
{
	int qd;
	int rc;
	Dev *dev;
	long (*xfswrite)(Usbfs *fs, Fid *f, void *data, long count, vlong );

	qd = qiddev(fid->qid.path);
	if(qd == Dtop)
		sysfatal("fswrite: not for usbd /");
	qlock(&fslck);
	if(fs[qd] == nil){
		qunlock(&fslck);
		werrstr(Eio);
		return -1;
	}
	dev = fs[qd]->dev;
	if(dev != nil)
		incref(dev);
	xfswrite = fs[qd]->write;
	qunlock(&fslck);
	rc = xfswrite(fs[qd], fid, data, cnt, off);
	if(dev != nil)
		closedev(dev);
	return rc;
}


static void
fsclunk(Usbfs*, Fid* fid)
{
	int qd;
	Dev *dev;
	void (*xfsclunk)(Usbfs *fs, Fid *f);

	dev = nil;
	qd = qiddev(fid->qid.path);
	qlock(&fslck);
	if(qd != Dtop && fs[qd] != nil){
		dev=fs[qd]->dev;
		if(dev != nil)
			incref(dev);
		xfsclunk = fs[qd]->clunk;
	}else
		xfsclunk = nil;
	qunlock(&fslck);
	if(xfsclunk != nil){
		xfsclunk(fs[qd], fid);
	}
	if(dev != nil)
		closedev(dev);
}

static int
fsstat(Usbfs*, Qid qid, Dir *d)
{
	int qd;
	int rc;
	Dev *dev;
	int (*xfsstat)(Usbfs *fs, Qid q, Dir *d);

	qd = qiddev(qid.path);
	if(qd == Dtop){
		d->qid = qid;
		d->name = "usb";
		d->length = 0;
		d->mode = 0555|DMDIR;
		return 0;
	}
	qlock(&fslck);
	if(fs[qd] == nil){
		qunlock(&fslck);
		werrstr(Eio);
		return -1;
	}
	xfsstat = fs[qd]->stat;
	dev = fs[qd]->dev;
	if(dev != nil)
		incref(dev);
	qunlock(&fslck);
	rc = xfsstat(fs[qd], qid, d);
	if(dev != nil)
		closedev(dev);
	return rc;
}

Usbfs usbdirfs =
{
	.walk = fswalk,
	.clone = fsclone,
	.clunk = fsclunk,
	.open = fsopen,
	.read = fsread,
	.write = fswrite,
	.stat = fsstat,
	.end = fsend,
};

