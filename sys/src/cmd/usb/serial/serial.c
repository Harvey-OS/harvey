#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "serial.h"
#include "prolific.h"
#include "ucons.h"
#include "ftdi.h"

/*
 * This part takes care of locking except for initialization and
 * other threads created by the hw dep. drivers.
 * BUG: An error on the device does not make the driver exit.
 * It probably should.
 */

int serialdebug;

enum {
	/* Qids. Maintain order (relative to dirtabs structs) */
	Qroot	= 0,
	Qctl,
	Qdata,
	Qmax,
};

typedef struct Dirtab Dirtab;
struct Dirtab {
	char	*name;
	int	mode;
};

static Dirtab dirtab[] = {
	[Qroot]	"/",	DMDIR|0555,
	[Qctl]	"ctl",	0444,
	[Qdata]	"data",	0640,
};

static int sdebug;


int
serialnop(Serial *)
{
	return 0;
}


int
serialnopctl(Serial *, int)
{
	return 0;
}


static void
serialfatal(Serial *ser)
{
	dsprint(2, "serial: fatal error, detaching\n");
	devctl(ser->dev, "detach");
	usbfsdel(&ser->fs);
}

/* I sleep with the lock... only way to drain in general*/
static void
serialdrain(Serial *ser)
{
	uint baud, pipesize;

	if(ser->maxwrite < 256)
		pipesize = 256;
	else
		pipesize = ser->maxwrite;
	baud = ser->baud;
	/* wait for the at least 256-byte pipe to clear */
	sleep(10 + pipesize/((1 + baud)*1000));
	if(ser->clearpipes != nil)
		ser->clearpipes(ser);
}

int
serialreset(Serial *ser)
{
	/* cmd for reset */
	serialdrain(ser);
	if(ser->reset != nil)
		ser->reset(ser);
	return 0;
}

/*call this if something goes wrong */
int
serialrecover(Serial *ser, char *err)
{
	if(strstr(err, "detached") != nil)
		return -1;
	if(ser->recover > 1)
		serialfatal(ser);
	ser->recover++;
	if(serialreset(ser) < 0)
		return -1;
	ser->recover = 0;
	return 0;
}

static int
serialctl(Serial *p, char *cmd)
{
	int c, i, n, nf, nop, nw, par, drain, set, lines;
	char *f[16];
	uchar x;

	drain = set = lines = 0;
	nf = tokenize(cmd, f, nelem(f));
	for(i = 0; i < nf; i++){
		if(strncmp(f[i], "break", 5) == 0){
			if(p->setbreak != nil)
				p->setbreak(p, 1);
			continue;
		}

		nop = 0;
		n = atoi(f[i]+1);
		c = *f[i];
		if (isascii(c) && isupper(c))
			c = tolower(c);
		switch(c){
		case 'b':
			drain++;
			p->baud = n;
			set++;
			break;
		case 'c':
			p->dcd = n;
			// lines++;
			++nop;
			break;
		case 'd':
			p->dtr = n;
			lines++;
			break;
		case 'e':
			p->dsr = n;
			// lines++;
			++nop;
			break;
		case 'f':		/* flush the pipes */
			drain++;
			break;
		case 'h':		/* hangup?? */
			p->rts = p->dtr = 0;
			lines++;
			fprint(2, "serial: %c, unsure ctl\n", c);
			break;
		case 'i':
			++nop;
			break;
		case 'k':
			drain++;
			p->setbreak(p, 1);
			sleep(n);
			p->setbreak(p, 0);
			break;
		case 'l':
			drain++;
			p->bits = n;
			set++;
			break;
		case 'm':
			drain++;
			if(p->modemctl != nil)
				p->modemctl(p, n);
			if(n == 0)
				p->cts = 0;
			break;
		case 'n':
			p->blocked = n;
			++nop;
			break;
		case 'p':		/* extended... */
			if(strlen(f[i]) != 2)
				return -1;
			drain++;
			par = f[i][1];
			if(par == 'n')
				p->parity = 0;
			else if(par == 'o')
				p->parity = 1;
			else if(par == 'e')
				p->parity = 2;
			else if(par == 'm')	/* mark parity */
				p->parity = 3;
			else if(par == 's')	/* space parity */
				p->parity = 4;
			else
				return -1;
			set++;
			break;
		case 'q':
			// drain++;
			p->limit = n;
			++nop;
			break;
		case 'r':
			drain++;
			p->rts = n;
			lines++;
			break;
		case 's':
			drain++;
			p->stop = n;
			set++;
			break;
		case 'w':
			/* ?? how do I put this */
			p->timer = n * 100000LL;
			++nop;
			break;
		case 'x':
			if(n == 0)
				x = CTLS;
			else
				x = CTLQ;
			if(p->wait4write != nil)
				nw = p->wait4write(p, &x, 1);
			else
				nw = write(p->epout->dfd, &x, 1);
			if(nw != 1){
				serialrecover(p, "");
				return -1;
			}
			break;
		}
		if (nop)
			fprint(2, "serial: %c, unsupported nop ctl\n", c);
	}
	if(drain)
		serialdrain(p);
	if(lines && !set){
		if(p->sendlines != nil && p->sendlines(p) < 0)
			return -1;
	} else if(set){
		if(p->setparam != nil && p->setparam(p) < 0)
			return -1;
	}
	return 0;
}

char *pformat = "noems";

char *
serdumpst(Serial *ser, char *buf, int bufsz)
{
	char *e, *s;

	e = buf + bufsz;
	s = seprint(buf, e, "b%d ", ser->baud);
	s = seprint(s, e, "c%d ", ser->dcd);	/* unimplemented */
	s = seprint(s, e, "d%d ", ser->dtr);
	s = seprint(s, e, "e%d ", ser->dsr);	/* unimplemented */
	s = seprint(s, e, "l%d ", ser->bits);
	s = seprint(s, e, "m%d ", ser->mctl);
	if(ser->parity >= 0 || ser->parity < strlen(pformat))
		s = seprint(s, e, "p%c ", pformat[ser->parity]);
	else
		s = seprint(s, e, "p%c ", '?');
	s = seprint(s, e, "r%d ", ser->rts);
	s = seprint(s, e, "s%d ", ser->stop);
	s = seprint(s, e, "i%d ", ser->fifo);
	s = seprint(s, e, "\ndev(%d) ", 0);
	s = seprint(s, e, "type(%d)  ", ser->type);
	s = seprint(s, e, "framing(%d) ", ser->nframeerr);
	s = seprint(s, e, "overruns(%d) ", ser->novererr);
	s = seprint(s, e, "berr(%d) ", ser->nbreakerr);
	s = seprint(s, e, " serr(%d) ", ser->nparityerr);
	return s;
}

static int
serinit(Serial *ser)
{
	int res;

	res = 0;
	if(ser->init != nil)
		res = ser->init(ser);
	if(ser->getparam != nil)
		ser->getparam(ser);
	ser->nframeerr = ser->nparityerr = ser->nbreakerr = ser->novererr = 0;
	return res;
}

static int
dwalk(Usbfs *fs, Fid *fid, char *name)
{
	int i;
	char *dname;
	Qid qid;
	Serial *ser;

	qid = fid->qid;
	if((qid.type & QTDIR) == 0){
		werrstr("walk in non-directory");
		return -1;
	}

	if(strcmp(name, "..") == 0){
		/* must be /eiaU%d; i.e. our root dir. */
		fid->qid.path = Qroot | fs->qid;
		fid->qid.vers = 0;
		fid->qid.type = QTDIR;
		return 0;
	}

	ser = fs->aux;
	for(i = 1; i < nelem(dirtab); i++){
		dname = smprint(dirtab[i].name, ser->fs.name);
		if(strcmp(name, dname) == 0){
			qid.path = i | fs->qid;
			qid.vers = 0;
			qid.type = dirtab[i].mode >> 24;
			fid->qid = qid;
			free(dname);
			return 0;
		} else
			free(dname);
	}
	werrstr(Enotfound);
	return -1;
}

static void
dostat(Usbfs *fs, int path, Dir *d)
{
	Dirtab *t;
	Serial *ser;

	t = &dirtab[path];
	d->qid.path = path;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
	ser = fs->aux;

	if(strcmp(t->name, "/") == 0)
		d->name = t->name;
	else
		snprint(d->name, Namesz, t->name, ser->fs.name);
	d->length = 0;
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
	// Serial *ser;

	path = fid->qid.path & ~fs->qid;
	// ser = fs->aux;
	switch(path){		/* BUG: unneeded? */
	case Qdata:
		dsprint(2, "serial, opened data\n");
		break;
	case Qctl:
		dsprint(2, "serial, opened ctl\n");
		break;
	}
	return 0;
}


static void
filldir(Usbfs *fs, Dir *d, Dirtab *tab, int i)
{
	d->qid.path = i | fs->qid;
	d->mode = tab->mode;
	if((d->mode & DMDIR) != 0)
		d->qid.type = QTDIR;
	else
		d->qid.type = QTFILE;
	d->name = tab->name;
}

static int
dirgen(Usbfs *fs, Qid, int i, Dir *d, void *)
{
	i++;				/* skip root */
	if(i >= nelem(dirtab))
		return -1;
	filldir(fs, d, &dirtab[i], i);
	return 0;
}

static long
dread(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	int dfd;
	long rcount;
	ulong path;
	char *e, *buf, *err;	/* change */
	Qid q;
	Serial *ser;

	q = fid->qid;
	path = fid->qid.path & ~fs->qid;
	ser = fs->aux;

	buf = emallocz(255, 1);
	err = emallocz(255, 1);
	qlock(ser);
	switch(path){
	case Qroot:
		count = usbdirread(fs, q, data, count, offset, dirgen, nil);
		break;
	case Qdata:
		if(count > ser->maxread)
			count = ser->maxread;
		dsprint(2, "serial: reading from data\n");
		do {
			err[0] = 0;
			dfd = ser->epin->dfd;
			if(usbdebug >= 3)
				dsprint(2, "serial: reading: %ld\n", count);

			if(ser->wait4data != nil)
				rcount = ser->wait4data(ser, data, count);
			else{
				qunlock(ser);
				rcount = read(dfd, data, count);
				qlock(ser);
			}
			if(rcount < 0)
				snprint(err, 255, "%r");
			if(usbdebug >= 3)
				dsprint(2, "serial: read: %s %ld\n", err, rcount);
		} while(rcount < 0 && strstr(err, "timed out") != nil);

		dsprint(2, "serial: read from bulk %ld, %10.10s\n", rcount, err);
		if(rcount < 0){
			dsprint(2, "serial: need to recover, data read %ld %r\n",
				count);
			serialrecover(ser, err);
		}
		dsprint(2, "serial: read from bulk %ld\n", rcount);
		count = rcount;
		break;
	case Qctl:
		if(offset != 0)
			count = 0;
		else {
			e = serdumpst(ser, buf, 255);
			count = usbreadbuf(data, count, 0, buf, e - buf);
		}
		break;
	}
	qunlock(ser);
	free(err);
	free(buf);
	return count;
}

static long
altwrite(Serial *ser, uchar *buf, long count)
{
	int nw, dfd;
	char err[128];

	do{
		if(ser->wait4write != nil)
			/* unlocked inside later */
			nw = ser->wait4write(ser, buf, count);
		else{
			dfd = ser->epout->dfd;
			qunlock(ser);
			nw = write(dfd, buf, count);
			qlock(ser);
		}
		rerrstr(err, sizeof err);
	} while(nw < 0 && strstr(err, "timed out") != nil);

	if(nw != count){
		dsprint(2, "serial: need to recover, status in write %d %r\n",
			nw);
		snprint(err, sizeof err, "%r");
		serialrecover(ser, err);
	}
	return nw;
}

static long
dwrite(Usbfs *fs, Fid *fid, void *buf, long count, vlong)
{
	ulong path;
	char *cmd;
	Serial *ser;

	ser = fs->aux;
	path = fid->qid.path & ~fs->qid;

	qlock(ser);
	switch(path){
	case Qdata:
		count = altwrite(ser, (uchar *)buf, count);
		break;
	case Qctl:
		cmd = emallocz(count+1, 1);
		memmove(cmd, buf, count);
		cmd[count] = 0;
		if(serialctl(ser, cmd) < 0){
			qunlock(ser);
			werrstr(Ebadctl);
			free(cmd);
			return -1;
		}
		free(cmd);
		break;
	default:
		qunlock(ser);
		werrstr(Eperm);
		return -1;
	}
	qunlock(ser);
	return count;
}

static int
openeps(Serial *ser, int epin, int epout, int epintr)
{
	ser->epin = openep(ser->dev, epin);
	if(ser->epin == nil){
		fprint(2, "serial: openep %d: %r\n", epin);
		return -1;
	}
	ser->epout = openep(ser->dev, epout);
	if(ser->epout == nil){
		fprint(2, "serial: openep %d: %r\n", epout);
		closedev(ser->epin);
		return -1;
	}

	devctl(ser->epin,  "timeout 1000");
	devctl(ser->epout, "timeout 1000");

	if(ser->hasepintr){
		ser->epintr = openep(ser->dev, epintr);
		if(ser->epintr == nil){
			fprint(2, "serial: openep %d: %r\n", epintr);
			closedev(ser->epin);
			closedev(ser->epout);
			return -1;
		}
		opendevdata(ser->epintr, OREAD);
		devctl(ser->epintr, "timeout 1000");
	}

	if(ser->seteps!= nil)
		ser->seteps(ser);
	opendevdata(ser->epin, OREAD);
	opendevdata(ser->epout, OWRITE);
	if(ser->epin->dfd < 0 || ser->epout->dfd < 0 ||
	    (ser->hasepintr && ser->epintr->dfd < 0)){
		fprint(2, "serial: open i/o ep data: %r\n");
		closedev(ser->epin);
		closedev(ser->epout);
		if(ser->hasepintr)
			closedev(ser->epintr);
		return -1;
	}
	return 0;
}

static int
findendpoints(Serial *ser)
{
	int i, epin, epout, epintr;
	Ep *ep, **eps;
	Usbdev *ud;

	epintr = epin = epout = -1;
	ud = ser->dev->usb;

	/*
	 * interfc 0 means start from the start which is equiv to
	 * iterate through endpoints probably, could be done better
	 */
	if(ser->interfc == 0)
		eps = ud->ep;
	else
		eps = ser->dev->usb->conf[0]->iface[ser->interfc]->ep;

	for(i = 0; i < Niface; i++){
		if((ep = eps[i]) == nil)
			continue;
		if(ser->hasepintr && ep->type == Eintr &&
		    ep->dir == Ein && epintr == -1)
			epintr = ep->id;
		if(ep->type == Ebulk){
			if(ep->dir == Ein && epin == -1)
				epin = ep->id;
			if(ep->dir == Eout && epout == -1)
				epout = ep->id;
		}
	}
	dprint(2, "serial: ep ids: in %d out %d intr %d\n", epin, epout, epintr);
	if(epin == -1 || epout == -1 || (ser->hasepintr && epintr == -1))
		return -1;

	if(openeps(ser, epin, epout, epintr) < 0)
		return -1;

	dprint(2, "serial: ep in %s out %s\n", ser->epin->dir, ser->epout->dir);
	if(ser->hasepintr)
		dprint(2, "serial: ep intr %s\n", ser->epintr->dir);

	if(usbdebug > 1 || serialdebug > 2){
		devctl(ser->epin,  "debug 1");
		devctl(ser->epout, "debug 1");
		if(ser->hasepintr)
			devctl(ser->epintr, "debug 1");
		devctl(ser->dev, "debug 1");
	}
	return 0;
}

/* keep in sync with main.c */
static int
usage(void)
{
	werrstr("usage: usb/serial [-dD] [-m mtpt] [-s srv]");
	return -1;
}

static void
serdevfree(void *a)
{
	Serial *ser = a;

	if(ser == nil)
		return;
	if(ser->hasepintr)
		closedev(ser->epintr);
	closedev(ser->epin);
	closedev(ser->epout);
	ser->epintr = ser->epin = ser->epout = nil;
	chanfree(ser->w4data);
	chanfree(ser->gotdata);
	chanfree(ser->w4empty);
	free(ser);
}

static Usbfs serialfs = {
	.walk =	dwalk,
	.open =	dopen,
	.read =	dread,
	.write=	dwrite,
	.stat =	dstat,
};

int
serialmain(Dev *dev, int argc, char* argv[])
{
	Serial *ser;
	char buf[50];

	ARGBEGIN{
	case 'd':
		serialdebug++;
		break;
	default:
		return usage();
	}ARGEND
	if(argc != 0)
		return usage();

	ser = dev->aux = emallocz(sizeof(Serial), 1);
	/* BUG could this go wrong?,channel leaks? */
	ser->w4data  = chancreate(sizeof(ulong), 0);
	ser->gotdata = chancreate(sizeof(ulong), 0);
	ser->w4empty = chancreate(sizeof(ulong), 0);
	ser->maxread = ser->maxwrite = 8*1024;
	ser->dev = dev;
	dev->free = serdevfree;

	snprint(buf, sizeof buf, "vid %#06x did %#06x",
		dev->usb->vid, dev->usb->did);
	ser->fs = serialfs;
	if(plmatch(buf) == 0){
		ser->hasepintr = 1;
		ser->Serialops = plops;
	} else if(uconsmatch(buf) == 0)
		ser->Serialops = uconsops;
	else if(ftmatch(ser, buf) == 0)
		ser->Serialops = ftops;
	else {
		werrstr("serial: no serial devices found");
		return -1;
	}

	if(findendpoints(ser) < 0){
		werrstr("serial: no endpoints found");
		return -1;
	}
	if(serinit(ser) < 0){
		dprint(2, "serial: serinit: %r\n");
		return -1;
	}

	snprint(ser->fs.name, sizeof ser->fs.name, "eiaU%d", dev->id);
	fprint(2, "%s\n", ser->fs.name);
	ser->fs.dev = dev;
	incref(dev);
	ser->fs.aux = ser;
	usbfsadd(&ser->fs);

	closedev(dev);
	return 0;
}
