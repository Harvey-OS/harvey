/*
 * This part takes care of locking except for initialization and
 * other threads created by the hw dep. drivers.
 */

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
#include "silabs.h"

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
	[Qroot]	"/",		DMDIR|0555,
	[Qdata]	"%s",		0660,
	[Qctl]	"%sctl",	0664,
};

static int sdebug;

static void
serialfatal(Serial *ser)
{
	Serialport *p;
	int i;

	dsprint(2, "serial: fatal error, detaching\n");
	devctl(ser->dev, "detach");

	for(i = 0; i < ser->nifcs; i++){
		p = &ser->p[i];
		usbfsdel(&p->fs);
		if(p->w4data != nil)
			chanclose(p->w4data);
		if(p->gotdata != nil)
			chanclose(p->gotdata);
		if(p->readc)
			chanclose(p->readc);
	}
}

/* I sleep with the lock... only way to drain in general */
static void
serialdrain(Serialport *p)
{
	Serial *ser;
	uint baud, pipesize;

	ser = p->s;
	baud = p->baud;

	if(p->baud == ~0)
		return;
	if(ser->maxwtrans < 256)
		pipesize = 256;
	else
		pipesize = ser->maxwtrans;
	/* wait for the at least 256-byte pipe to clear */
	sleep(10 + pipesize/((1 + baud)*1000));
	if(ser->clearpipes != nil)
		ser->clearpipes(p);
}

int
serialreset(Serial *ser)
{
	Serialport *p;
	int i, res;

	res = 0;
	/* cmd for reset */
	for(i = 0; i < ser->nifcs; i++){
		p = &ser->p[i];
		serialdrain(p);
	}
	if(ser->reset != nil)
		res = ser->reset(ser, nil);
	return res;
}

/* call this if something goes wrong, must be qlocked */
int
serialrecover(Serial *ser, Serialport *p, Dev *ep, char *err)
{
	if(p != nil)
		dprint(2, "serial[%d], %s: %s, level %d\n", p->interfc,
			p->name, err, ser->recover);
	else
		dprint(2, "serial[%s], global error, level %d\n",
			ser->p[0].name, ser->recover);
	ser->recover++;
	if(strstr(err, "detached") != nil)
		return -1;
	if(ser->recover < 3){
		if(p != nil){	
			if(ep != nil){
				if(ep == p->epintr)
					unstall(ser->dev, p->epintr, Ein);
				if(ep == p->epin)
					unstall(ser->dev, p->epin, Ein);
				if(ep == p->epout)
					unstall(ser->dev, p->epout, Eout);
				return 0;
			}

			if(p->epintr != nil)
				unstall(ser->dev, p->epintr, Ein);
			if(p->epin != nil)
				unstall(ser->dev, p->epin, Ein);
			if(p->epout != nil)
				unstall(ser->dev, p->epout, Eout);
		}
		return 0;
	}
	if(ser->recover > 4 && ser->recover < 8)
		serialfatal(ser);
	if(ser->recover > 8){
		ser->reset(ser, p);
		return 0;
	}
	if(serialreset(ser) < 0)
		return -1;
	return 0;
}

static int
serialctl(Serialport *p, char *cmd)
{
	Serial *ser;
	int c, i, n, nf, nop, nw, par, drain, set, lines;
	char *f[16];
	uchar x;

	ser = p->s;
	drain = set = lines = 0;
	nf = tokenize(cmd, f, nelem(f));
	for(i = 0; i < nf; i++){
		if(strncmp(f[i], "break", 5) == 0){
			if(ser->setbreak != nil)
				ser->setbreak(p, 1);
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
			ser->setbreak(p, 1);
			sleep(n);
			ser->setbreak(p, 0);
			break;
		case 'l':
			drain++;
			p->bits = n;
			set++;
			break;
		case 'm':
			drain++;
			if(ser->modemctl != nil)
				ser->modemctl(p, n);
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
			if(ser->wait4write != nil)
				nw = ser->wait4write(p, &x, 1);
			else
				nw = write(p->epout->dfd, &x, 1);
			if(nw != 1){
				serialrecover(ser, p, p->epout, "");
				return -1;
			}
			break;
		}
		/*
		 * don't print.  the condition is harmless and the print
		 * splatters all over the display.
		 */
		USED(nop);
		if (0 && nop)
			fprint(2, "serial: %c, unsupported nop ctl\n", c);
	}
	if(drain)
		serialdrain(p);
	if(lines && !set){
		if(ser->sendlines != nil && ser->sendlines(p) < 0)
			return -1;
	} else if(set){
		if(ser->setparam != nil && ser->setparam(p) < 0)
			return -1;
	}
	ser->recover = 0;
	return 0;
}

char *pformat = "noems";

char *
serdumpst(Serialport *p, char *buf, int bufsz)
{
	char *e, *s;
	Serial *ser;

	ser = p->s;

	e = buf + bufsz;
	s = seprint(buf, e, "b%d ", p->baud);
	s = seprint(s, e, "c%d ", p->dcd);	/* unimplemented */
	s = seprint(s, e, "d%d ", p->dtr);
	s = seprint(s, e, "e%d ", p->dsr);	/* unimplemented */
	s = seprint(s, e, "l%d ", p->bits);
	s = seprint(s, e, "m%d ", p->mctl);
	if(p->parity >= 0 || p->parity < strlen(pformat))
		s = seprint(s, e, "p%c ", pformat[p->parity]);
	else
		s = seprint(s, e, "p%c ", '?');
	s = seprint(s, e, "r%d ", p->rts);
	s = seprint(s, e, "s%d ", p->stop);
	s = seprint(s, e, "i%d ", p->fifo);
	s = seprint(s, e, "\ndev(%d) ", 0);
	s = seprint(s, e, "type(%d)  ", ser->type);
	s = seprint(s, e, "framing(%d) ", p->nframeerr);
	s = seprint(s, e, "overruns(%d) ", p->novererr);
	s = seprint(s, e, "berr(%d) ", p->nbreakerr);
	s = seprint(s, e, " serr(%d)\n", p->nparityerr);
	return s;
}

static int
serinit(Serialport *p)
{
	int res;
	res = 0;
	Serial *ser;

	ser = p->s;

	if(ser->init != nil)
		res = ser->init(p);
	if(ser->getparam != nil)
		ser->getparam(p);
	p->nframeerr = p->nparityerr = p->nbreakerr = p->novererr = 0;

	return res;
}

static int
dwalk(Usbfs *fs, Fid *fid, char *name)
{
	int i;
	char *dname;
	Qid qid;
	Serialport *p;

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

	p = fs->aux;
	for(i = 1; i < nelem(dirtab); i++){
		dname = smprint(dirtab[i].name, p->name);
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
	Serialport *p;

	t = &dirtab[path];
	d->qid.path = path;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
	p = fs->aux;

	if(strcmp(t->name, "/") == 0)
		d->name = t->name;
	else
		snprint(d->name, Namesz, t->name, p->fs.name);
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
	Serialport *p;

	path = fid->qid.path & ~fs->qid;
	p = fs->aux;
	switch(path){		/* BUG: unneeded? */
	case Qdata:
		dsprint(2, "serial, opened data\n");
		break;
	case Qctl:
		dsprint(2, "serial, opened ctl\n");
		if(p->isjtag)
			return 0;
		serialctl(p, "l8 i1");	/* default line parameters */
		break;
	}
	return 0;
}


static void
filldir(Usbfs *fs, Dir *d, Dirtab *tab, int i, void *v)
{
	Serialport *p;

	p = v;
	d->qid.path = i | fs->qid;
	d->mode = tab->mode;
	if((d->mode & DMDIR) != 0)
		d->qid.type = QTDIR;
	else
		d->qid.type = QTFILE;
	sprint(d->name, tab->name, p->name);	/* hope it fits */
}

static int
dirgen(Usbfs *fs, Qid, int i, Dir *d, void *p)
{
	i++;				/* skip root */
	if(i >= nelem(dirtab))
		return -1;
	filldir(fs, d, &dirtab[i], i, p);
	return 0;
}

enum {
	Serbufsize	= 255,
};

static long
dread(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	int dfd;
	long rcount;
	ulong path;
	char *e, *buf, *err;	/* change */
	Qid q;
	Serialport *p;
	Serial *ser;
	static int errrun, good;

	q = fid->qid;
	path = fid->qid.path & ~fs->qid;
	p = fs->aux;
	ser = p->s;

	buf = emallocz(Serbufsize, 1);
	err = emallocz(Serbufsize, 1);
	qlock(ser);
	switch(path){
	case Qroot:
		count = usbdirread(fs, q, data, count, offset, dirgen, p);
		break;
	case Qdata:
		if(count > ser->maxread)
			count = ser->maxread;
		dsprint(2, "serial: reading from data\n");
		do {
			err[0] = 0;
			dfd = p->epin->dfd;
			if(usbdebug >= 3)
				dsprint(2, "serial: reading: %ld\n", count);

			assert(count > 0);
			if(ser->wait4data != nil)
				rcount = ser->wait4data(p, data, count);
			else{
				qunlock(ser);
				rcount = read(dfd, data, count);
				qlock(ser);
			}
			/*
			 * if we encounter a long run of continuous read
			 * errors, do something drastic so that our caller
			 * doesn't just spin its wheels forever.
			 */
			if(rcount < 0) {
				snprint(err, Serbufsize, "%r");
				++errrun;
				sleep(20);
				if (good > 0 && errrun > 10000) {
					/* the line has been dropped; give up */
					qunlock(ser);
					fprint(2, "%s: line %s is gone: %r\n",
						argv0, p->fs.name);
					threadexitsall("serial line gone");
				}
			} else {
				errrun = 0;
				good++;
			}
			if(usbdebug >= 3)
				dsprint(2, "serial: read: %s %ld\n", err, rcount);
		} while(rcount < 0 && strstr(err, "timed out") != nil);

		dsprint(2, "serial: read from bulk %ld, %10.10s\n", rcount, err);
		if(rcount < 0){
			dsprint(2, "serial: need to recover, data read %ld %r\n",
				count);
			serialrecover(ser, p, p->epin, err);
		}
		dsprint(2, "serial: read from bulk %ld\n", rcount);
		count = rcount;
		break;
	case Qctl:
		if(offset != 0 || p->isjtag)
			count = 0;
		else {
			e = serdumpst(p, buf, Serbufsize);
			count = usbreadbuf(data, count, 0, buf, e - buf);
		}
		break;
	}
	if(count >= 0)
		ser->recover = 0;
	qunlock(ser);
	free(err);
	free(buf);
	return count;
}

static long
altwrite(Serialport *p, uchar *buf, long count)
{
	int nw, dfd;
	char err[128];
	Serial *ser;

	ser = p->s;
	do{
		dsprint(2, "serial: write to bulk %ld\n", count);

		if(ser->wait4write != nil)
			/* unlocked inside later */
			nw = ser->wait4write(p, buf, count);
		else{
			dfd = p->epout->dfd;
			qunlock(ser);
			nw = write(dfd, buf, count);
			qlock(ser);
		}
		rerrstr(err, sizeof err);
		dsprint(2, "serial: written %s %d\n", err, nw);
	} while(nw < 0 && strstr(err, "timed out") != nil);

	if(nw != count){
		dsprint(2, "serial: need to recover, status in write %d %r\n",
			nw);
		snprint(err, sizeof err, "%r");
		serialrecover(p->s, p, p->epout, err);
	}
	return nw;
}

static long
dwrite(Usbfs *fs, Fid *fid, void *buf, long count, vlong)
{
	ulong path;
	char *cmd;
	Serialport *p;
	Serial *ser;

	p = fs->aux;
	ser = p->s;
	path = fid->qid.path & ~fs->qid;

	qlock(ser);
	switch(path){
	case Qdata:
		count = altwrite(p, (uchar *)buf, count);
		break;
	case Qctl:
		if(p->isjtag)
			break;
		cmd = emallocz(count+1, 1);
		memmove(cmd, buf, count);
		cmd[count] = 0;
		if(serialctl(p, cmd) < 0){
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
	if(count >= 0)
		ser->recover = 0;
	else
		serialrecover(ser, p, p->epout, "writing");
	qunlock(ser);
	return count;
}

static int
openeps(Serialport *p, int epin, int epout, int epintr)
{
	Serial *ser;

	ser = p->s;
	p->epin = openep(ser->dev, epin);
	if(p->epin == nil){
		fprint(2, "serial: openep %d: %r\n", epin);
		return -1;
	}
	if(epout == epin){
		incref(p->epin);
		p->epout = p->epin;
	}else
		p->epout = openep(ser->dev, epout);
	if(p->epout == nil){
		fprint(2, "serial: openep %d: %r\n", epout);
		closedev(p->epin);
		return -1;
	}

	if(!p->isjtag){
		devctl(p->epin,  "timeout 1000");
		devctl(p->epout, "timeout 1000");
	}

	if(ser->hasepintr){
		p->epintr = openep(ser->dev, epintr);
		if(p->epintr == nil){
			fprint(2, "serial: openep %d: %r\n", epintr);
			closedev(p->epin);
			closedev(p->epout);
			return -1;
		}
		opendevdata(p->epintr, OREAD);
		devctl(p->epintr, "timeout 1000");
	}

	if(ser->seteps!= nil)
		ser->seteps(p);
	if(p->epin == p->epout)
		opendevdata(p->epin, ORDWR);
	else{
		opendevdata(p->epin, OREAD);
		opendevdata(p->epout, OWRITE);
	}
	if(p->epin->dfd < 0 ||p->epout->dfd < 0 ||
	    (ser->hasepintr && p->epintr->dfd < 0)){
		fprint(2, "serial: open i/o ep data: %r\n");
		closedev(p->epin);
		closedev(p->epout);
		if(ser->hasepintr)
			closedev(p->epintr);
		return -1;
	}
	return 0;
}

static int
findendpoints(Serial *ser, int ifc)
{
	int i, epin, epout, epintr;
	Ep *ep, **eps;

	epintr = epin = epout = -1;

	/*
	 * interfc 0 means start from the start which is equiv to
	 * iterate through endpoints probably, could be done better
	 */
	eps = ser->dev->usb->conf[0]->iface[ifc]->ep;

	for(i = 0; i < Niface; i++){
		if((ep = eps[i]) == nil)
			continue;
		if(ser->hasepintr && ep->type == Eintr &&
		    ep->dir == Ein && epintr == -1)
			epintr = ep->id;
		if(ep->type == Ebulk){
			if((ep->dir == Ein || ep->dir == Eboth) && epin == -1)
				epin = ep->id;
			if((ep->dir == Eout || ep->dir == Eboth) && epout == -1)
				epout = ep->id;
		}
	}
	dprint(2, "serial[%d]: ep ids: in %d out %d intr %d\n", ifc, epin, epout, epintr);
	if(epin == -1 || epout == -1 || (ser->hasepintr && epintr == -1))
		return -1;

	if(openeps(&ser->p[ifc], epin, epout, epintr) < 0)
		return -1;

	dprint(2, "serial: ep in %s out %s\n", ser->p[ifc].epin->dir, ser->p[ifc].epout->dir);
	if(ser->hasepintr)
		dprint(2, "serial: ep intr %s\n", ser->p[ifc].epintr->dir);

	if(usbdebug > 1 || serialdebug > 2){
		devctl(ser->p[ifc].epin,  "debug 1");
		devctl(ser->p[ifc].epout, "debug 1");
		if(ser->hasepintr)
			devctl(ser->p[ifc].epintr, "debug 1");
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
	Serialport *p;
	int i;

	if(ser == nil)
		return;

	for(i = 0; i < ser->nifcs; i++){
		p = &ser->p[i];

		if(ser->hasepintr)
			closedev(p->epintr);
		closedev(p->epin);
		closedev(p->epout);
		p->epintr = p->epin = p->epout = nil;
		if(p->w4data != nil)
			chanfree(p->w4data);
		if(p->gotdata != nil)
			chanfree(p->gotdata);
		if(p->readc)
			chanfree(p->readc);

	}
	free(ser);
}

static Usbfs serialfs = {
	.walk =	dwalk,
	.open =	dopen,
	.read =	dread,
	.write=	dwrite,
	.stat =	dstat,
};

static void
serialfsend(Usbfs *fs)
{
	Serialport *p;

	p = fs->aux;

	if(p->w4data != nil)
		chanclose(p->w4data);
	if(p->gotdata != nil)
		chanclose(p->gotdata);
	if(p->readc)
		chanclose(p->readc);
}

int
serialmain(Dev *dev, int argc, char* argv[])
{
	Serial *ser;
	Serialport *p;
	char buf[50];
	int i, devid;

	devid = dev->id;
	ARGBEGIN{
	case 'd':
		serialdebug++;
		break;
	case 'N':
		devid = atoi(EARGF(usage()));
		break;
	default:
		return usage();
	}ARGEND
	if(argc != 0)
		return usage();

	ser = dev->aux = emallocz(sizeof(Serial), 1);
	ser->maxrtrans = ser->maxwtrans = sizeof ser->p[0].data;
	ser->maxread = ser->maxwrite = sizeof ser->p[0].data;
	ser->dev = dev;
	dev->free = serdevfree;
	ser->jtag = -1;
	ser->nifcs = 1;

	snprint(buf, sizeof buf, "vid %#06x did %#06x",
		dev->usb->vid, dev->usb->did);
	if(plmatch(buf) == 0){
		ser->hasepintr = 1;
		ser->Serialops = plops;
	} else if(uconsmatch(buf) == 0)
		ser->Serialops = uconsops;
	else if(ftmatch(ser, buf) == 0)
		ser->Serialops = ftops;
	else if(slmatch(buf) == 0)
		ser->Serialops = slops;
	else {
		werrstr("serial: no serial devices found");
		return -1;
	}
	for(i = 0; i < ser->nifcs; i++){
		p = &ser->p[i];
		p->interfc = i;
		p->s = ser;
		p->fs = serialfs;
		if(i == ser->jtag){
			p->isjtag++;
		}
		if(findendpoints(ser, i) < 0){
			werrstr("serial: no endpoints found for ifc %d", i);
			return -1;
		}
		p->w4data  = chancreate(sizeof(ulong), 0);
		p->gotdata = chancreate(sizeof(ulong), 0);
	}

	qlock(ser);
	serialreset(ser);
	for(i = 0; i < ser->nifcs; i++){
		p = &ser->p[i];
		dprint(2, "serial: valid interface, calling serinit\n");
		if(serinit(p) < 0){
			dprint(2, "serial: serinit: %r\n");
			return -1;
		}

		dsprint(2, "serial: adding interface %d, %p\n", p->interfc, p);
		if(p->isjtag){
			snprint(p->name, sizeof p->name, "jtag");
			dsprint(2, "serial: JTAG interface %d %p\n", i, p);
			snprint(p->fs.name, sizeof p->fs.name, "jtag%d.%d", devid, i);
		} else {
			snprint(p->name, sizeof p->name, "eiaU");
			if(i == 0)
				snprint(p->fs.name, sizeof p->fs.name, "eiaU%d", devid);
			else
				snprint(p->fs.name, sizeof p->fs.name, "eiaU%d.%d", devid, i);
		}
		fprint(2, "%s...", p->fs.name);
		p->fs.dev = dev;
		incref(dev);
		p->fs.aux = p;
		p->fs.end = serialfsend;
		usbfsadd(&p->fs);
	}

	qunlock(ser);
	return 0;
}
