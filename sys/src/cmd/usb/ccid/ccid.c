#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <thread.h>
#include "le.h"
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "eprpc.h"
#include "atr.h"
#include "tagrd.h"

int cciddebug;

enum {
	/* Qids. Maintain order (relative to dirtabs structs) */
	Qroot	= 0,
	Qraw,
	Qrpc,
	Qctl,
	Qmax,
};

typedef struct Dirtab Dirtab;
struct Dirtab {
	char	*name;
	int	mode;
};

static Dirtab dirtab[] = {
	[Qroot]	"/",	DMDIR|0555,
	[Qraw]	"raw",	DMEXCL|0444,
	[Qrpc]	"rpc",	0640,
	[Qctl]	"ctl",	0640,
};

static int sdebug;

static int
ccidnop(Ccid *){
	return 0;
}

Iccops defops = {
	.init = ccidnop,
};


static void
ccidfatal(Ccid *ccid)
{
	dcprint(2, "ccid: fatal error, detaching\n");
	devctl(ccid->dev, "detach");
	usbfsdel(&ccid->fs);
}

int
ccidabort(Ccid *ser, uchar bseq, uchar bslot, uchar *buf)
{
	int res;
	int val;
	int iface;

	iface = 0;

	val = (bseq<<8) | bslot;
	dcprint(2, "serial: ccidabort val: 0x%x idx:%d buf:%p\n",
		val, iface, buf);
	res = usbcmd(ser->dev,   Rh2d |  Rclass | Rdev, CcidAbortcmd,
		val, iface, nil, 0);
	dcprint(2, "serial: ccidabort res:%d\n", res);
	return res;
}


static int
ccidreset(Ccid *)
{

	fprint(2, "ccid: reset unimplemented\n");
	return -1;
}

/*call this if something goes wrong */
int
ccidrecover(Ccid *ccid, char *err)
{
	Cmsg *c;

	fprint(2, "ccid: recover\n");
	if(strstr(err, "detached") != nil)
		return -1;
	if(ccid->recover == 0){
		if(ccid->sl != nil){
			ccidabort(ccid, ccid->sl->nreq, 0, nil);
			c = iccmsgrpc(ccid, nil, 0, AbortTyp, 0);
			free(c);
		}
		ccid->recover++;
		return 0;
	}
	else if(ccid->recover == 1){	
		ccidreset(ccid);
		ccid->recover++;
		return 0;
	}
	else if(ccid->recover >= 2){
		ccidfatal(ccid);
		return -1;
	}
	ccid->recover = 0;
	return 0;
}



static int
dwalk(Usbfs *fs, Fid *fid, char *name)
{
	int i;
	char *dname;
	Qid qid;
	Ccid *ccid;

	qid = fid->qid;
	if((qid.type & QTDIR) == 0){
		werrstr("walk in non-directory");
		return -1;
	}

	if(strcmp(name, "..") == 0){
		/* must be /ccidU%d; i.e. our root dir. */
		fid->qid.path = Qroot | fs->qid;
		fid->qid.vers = 0;
		fid->qid.type = QTDIR;
		return 0;
	}

	ccid = fs->aux;
	for(i = 1; i < nelem(dirtab); i++){
		dname = smprint(dirtab[i].name, ccid->fs.name);
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
	Ccid *ccid;

	t = &dirtab[path];
	d->qid.path = path;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
	ccid = fs->aux;

	if(strcmp(t->name, "/") == 0)
		d->name = t->name;
	else
		snprint(d->name, Namesz, t->name, ccid->fs.name);

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
	Ccid *ccid;
	Client *cl;
	int i;

	path = fid->qid.path & ~fs->qid;
	ccid = fs->aux;
	qlock(ccid);
	switch(path){	
	case Qrpc:
		if(ccid->israwopen){
			dcprint(2, "ccid: opening rpc device in use\n");
			goto Error;
		}
		dcprint(2, "ccid, opened data\n");
		ccid->isrpcopen++;
		cl = emallocz(sizeof (Client), 1);	/* will be filled when atr is known */
		fid->aux = cl;
		cl->ccid = ccid;
		
		for(i = 0; i < Maxcli; i++){
			if(ccid->cl[i] == nil)
				break;
		}
		if(i == Maxcli)
			sysfatal("too many clients");
		ccid->cl[i] = cl;
		cl->ncli = i;
		break;
	case Qraw:
		if(ccid->israwopen || ccid->isrpcopen){
			dcprint(2, "ccid: opening raw device in use\n");
			goto Error;
		}
		dcprint(2, "ccid, opened raw\n");
		ccid->israwopen++;
		break;
	case Qctl:
		break;
	}
	qunlock(ccid);
	return 0;
Error:
	werrstr(Einuse);
	qunlock(ccid);
	return -1;
}

static void
dclunk(Usbfs *fs, Fid *fid)
{

	ulong path;
	Ccid *ccid;
	Client *cl;


	if(fid->omode == ONONE)
		return;
	path = fid->qid.path & ~fs->qid;
	ccid = fs->aux;
	qlock(ccid);
	cl = fid->aux;
	switch(path){	
	case Qrpc:
		assert(ccid->isrpcopen);
		ccid->isrpcopen--;
		if(cl && cl->sl != nil){
			sendp(cl->to, nil);
		}
		break;
	case Qraw:
		assert(ccid->israwopen);
		fid->aux = nil; /* tell the fid to exit... */
		ccid->israwopen--;
		break;
	case Qctl:
		break;
	}
	qunlock(ccid);
	return;
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
	Dirtab *tab;

	i++;				/* skip root */
	if(i < nelem(dirtab))
		tab = &dirtab[i];
	else
		return -1;
	filldir(fs, d, tab, i);
	return 0;
}

char *voltsup[] = {
	[DVolt5] = "5.00",
	[DVolt3] = "3.00",
	[DVolt1_8] = "1.80",
};

static int
ccidprintdesc(Ccid *dd, char *buf, int maxsz)
{
	char *s, *e;
	int v;

	s = buf;
	e = s + maxsz;
	
	s = seprint(s, e, "release: %1.1x.%1.1x\n", dd->release&0xf0, dd->release&0x0f);
	s = seprint(s, e, "maxslot: %ud\n", dd->maxslot);
	s = seprint(s, e, "voltsup: %ub\\B\n", dd->voltsup);
	for(v = 1; v < DVoltMax; v <<=1)
		s = seprint(s, e, "	%4.4s: %ud", voltsup[v], (dd->voltsup&v) != 0);
	s = seprint(s, e, "\nprotomsk %lub\\B\n", dd->protomsk);
	s = seprint(s, e, "	T0: %d, T1: %d\n", (0x1&dd->protomsk) != 0, (0x2&dd->protomsk) !=0);
	s = seprint(s, e, "defclock: %lud\n", dd->defclock);
	s = seprint(s, e, "maxclock: %lud\n", dd->maxclock);
	s = seprint(s, e, "nclocks: %ud\n", dd->nclocks);
	s = seprint(s, e, "datarate: %lud\n", dd->datarate);
	s = seprint(s, e, "maxdatarate: %lud\n", dd->maxdatarate);
	s = seprint(s, e, "ndatarates: %ud\n", dd->ndatarates);
	s = seprint(s, e, "maxifsd: %lud\n", dd->maxifsd);
	s = seprint(s, e, "synchproto: %lub\\B\n", dd->synchproto);
	s = seprint(s, e, "	2_wire: %ud 3_wire: %ud I2C: %ud\n",
		(dd->synchproto&0x1) != 0, (dd->synchproto&0x2) != 0, (dd->synchproto&0x4) !=0);
	s = seprint(s, e, "mechan: %lub\\B\n", dd->mechanical);
	s = seprint(s, e, "	acc: %ud rej: %ud eje: %ud lck: %ud\n", 
			(dd->mechanical&0x1) != 0, (dd->mechanical&0x2) != 0,
			(dd->mechanical&0x3) != 0, (dd->mechanical&0x4) != 0);
	s = seprint(s, e, "features: %#8.8lux\n", dd->features);
	return s - buf;
}


static char *
dumpccid(Ccid *ccid, char *buf,int bufsz)
{
	char *e, *s;
	Dev *dev;

	dev = ccid->dev;
	e = buf + bufsz;
	s = seprint(buf, e, "vid %#06x did %#06x\n",
			dev->usb->vid, dev->usb->did);
	s = seprint(s, e, "rec %d\n", ccid->recover);
	s += ccidprintdesc(ccid, s, e - s);
	return s;
}

static long
dread(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	long rcount;
	ulong path;
	char *buf;	
	char *e;
	Qid q;
	Ccid *ccid;
	int dfd;
	Client *cl;
	Cmsg *c;

	rcount = 0;
	q = fid->qid;
	path = fid->qid.path & ~fs->qid;
	ccid = fs->aux;
	buf = nil;
	qlock(ccid);
	cl = fid->aux;
	switch(path){
	case Qroot:
		rcount = usbdirread(fs, q, data, count, offset, dirgen, nil);
		break;
	case Qrpc:
		dcprint(2, "ccid: reading from data\n");
		if(cl->sl == nil){
			werrstr("scard not present");
			qunlock(ccid);
			return -1;
		}
		if(cl->halfrpc != 1){
			werrstr("read but should be write");
			qunlock(ccid);
			return -1;
		}
		qunlock(ccid);
		c = recvp(cl->from);
		qlock(ccid);
		dcprint(2, "ccid: unblocked by the client proc\n");
		cl->halfrpc = 0;
		if(c == nil){
			werrstr("bad rpc"); 
			qunlock(ccid);
			rcount = -1;
		}
		else{
			if(c->errstr != nil){
				werrstr(c->errstr); 
				free(c->errstr);
				free(c);
				qunlock(ccid);
				return -1;
			}
			if(c->len > count){
				werrstr("read too small, %#lux > %#lux", c->len, count); /* BUG? return half msg?*/
				qunlock(ccid);
				return -1;
			}
			dcprint(2, "ccid: rcv msg: %lud\n", c->len);
			rcount = c->len;
			if(c->data != nil)
				memmove(data, c->data, c->len);
			free(c);
		}
		dcprint(2, "ccid: done, finished %lud\n", rcount);

		break;
	case Qctl:
		buf = emallocz(Dmaxstr + 255, 1);	/* at least ... */
		if(offset != 0){
			rcount = 0;
			break;
		}
		e = dumpccid(ccid, buf, Dmaxstr + 255);
		rcount = usbreadbuf(data, count, 0, buf, e - buf);
		break;
	case Qraw:
		qunlock(ccid);
		dfd = ccid->epin->dfd;
		rcount = read(dfd, data, count);
		qlock(ccid);
		break;
	}
	qunlock(ccid);
	free(buf);
	return rcount;
}

static long
dwrite(Usbfs *fs, Fid *fid, void *buf, long count, vlong)
{
	int nw, hastowait;
	ulong path;
	char err[40];
	Ccid *ccid;
	Client *cl;
	Cmsg *c;
	uchar *buffer;

	buffer = buf;
	ccid = fs->aux;
	path = fid->qid.path & ~fs->qid;
	cl = fid->aux;

	qlock(ccid);
	switch(path){
	case Qrpc:
		if(cl->sl == nil){
			if(count <=1 || (buffer[0] != 0x3b && buffer[0] != 0x3f)){	/* sanity check */
				if(count <= 1)
					werrstr("expected an ATR");
				else
					werrstr("expected an ATR got %#2.2ux", buffer[0]);
				goto Error;
			}
			cl = fillclient(cl, buffer, count);
			if(ccid->sl && matchcard(ccid, buffer, count)){
				cl->sl = ccid->sl;
				hastowait = 0;
			}
			else
				hastowait = 1;
			qunlock(ccid);

			/* got a new atr, should match it to existing card */
			if(hastowait)
				recvp(cl->start);
			qlock(ccid);
		}
		else{
			/* second write */
			if(cl->sl == nil){
				werrstr("scard went away");
				goto Error;
			}
			if(count > Maxpayload){
				werrstr("apdu too big");
				goto Error;
			}
			
			c = mallocz(sizeof (Cmsg), 1);
			if(count == 0){
				werrstr("hdr small"); /*technically not an error, aids debugging */
				c->isabort = 1;
			}
			else{
				if(cl->halfrpc == 1){
					werrstr("protocol botch");
					goto Error;
				}
				c->data = c->unpkdata;
				memmove(c->data, buf, count);
				c->len = count;
				dcprint(2, "tagrd: sending to client\n");
				sendp(cl->to, c);
				cl->halfrpc = 1;
			}
		}
		break;
	case Qctl:
		werrstr(Ebadctl);
		goto Error;
	case Qraw:
		nw = write(ccid->epout->dfd, buf, count);
		if(nw != count){
			dcprint(2, "ccid: need to recover, status read %d %r\n",
				nw);
			snprint(err, sizeof err, "%r");
			ccidrecover(ccid, err);
		}
		count = nw;
		break;
	default:
		werrstr(Eperm);
		goto Error;
	}
	qunlock(ccid);
	return count;
Error:
	qunlock(ccid);
	return -1;
}

static int
openeps(Ccid *ccid, int epin, int epout, int epintr, int isboth)
{
	ccid->epin = openep(ccid->dev, epin);
	if(ccid->epin == nil){
		fprint(2, "ccid: openep %d: %r\n", epin);
		return -1;
	}

	if(!isboth){	
		ccid->epout = openep(ccid->dev, epout);
		if(ccid->epout == nil){
			fprint(2, "ccid: openep %d: %r\n", epout);
			closedev(ccid->epin);
			return -1;
		}
	}

	if(ccid->hasintr){
		ccid->epintr = openep(ccid->dev, epintr);
		if(ccid->epintr == nil){
			fprint(2, "ccid: openep %d: %r\n", epintr);
			closedev(ccid->epin);
			closedev(ccid->epout);
			return -1;
		}
	}

	if(!isboth){
		opendevdata(ccid->epin, OREAD);
		opendevdata(ccid->epout, OWRITE);
	}
	else{
		opendevdata(ccid->epin, ORDWR);
		ccid->epout = ccid->epin;
		incref(ccid->epin);
	}
	if(ccid->epin->dfd < 0 || ccid->epout->dfd < 0){
		goto Err;
	}
	if(ccid->hasintr){	
		opendevdata(ccid->epintr, OREAD);
		if(ccid->epintr->dfd < 0){
			closedev(ccid->epintr);
			goto Err;
		}
	}
	return 0;
Err:
		fprint(2, "ccid: open i/o ep data: %r\n");
		closedev(ccid->epin);
		closedev(ccid->epout);
		return -1;
}

static int
findendpoints(Ccid *ccid)
{
	int i, epin, epout, epintr;
	Ep *ep;
	Usbdev *ud;
	int isboth;

	isboth = 0;
	epintr = epin = epout = -1;
	ud = ccid->dev->usb;
	for(i = 0; i < nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			continue;
		if(ep->type == Eintr && ep->dir == Ein && epintr == -1)
			epintr = ep->id;
		if(ep->type == Ebulk){
			if(ep->dir == Eboth && epin == -1 && epout == -1){
				isboth++;
				epin = ep->id;
				epout = ep->id;
			}		
			if(ep->dir == Ein && epin == -1)
				epin = ep->id;
			if(ep->dir == Eout && epout == -1)
				epout = ep->id;
		}
	}
	dprint(2, "ccid: ep ids: in %d out %d intr %d\n", epin, epout, epintr);

	 if(epintr != -1)
		ccid->hasintr = 1;
	if(epin == -1 || epout == -1)
		return -1;
	if(openeps(ccid, epin, epout, epintr, isboth) < 0)
		return -1;

	dprint(2, "ccid: ep in %s out %s intr %s\n",
		ccid->epin->dir, ccid->epout->dir, ccid->hasintr?ccid->epintr->dir:"nointr");

	if(usbdebug > 1 || cciddebug > 2){
		devctl(ccid->epin, "debug 1");
		devctl(ccid->epout, "debug 1");
		if(ccid->hasintr)
			devctl(ccid->epintr, "debug 1");
		devctl(ccid->dev, "debug 1");
	}
	return 0;
}

static int
ccidgetdesc(Ccid *ccid, DDesc *dd)
{
	uchar *p;
	p = dd->bbytes;

	ccid->release = иhgets(p);
	p += sizeof(ccid->release);

	ccid->maxslot = *p++;
	ccid->voltsup = *p++;

	ccid->protomsk = иhgetl(p);
	p += sizeof(ccid->protomsk);
	ccid->defclock = иhgetl(p);
	p += sizeof(ccid->defclock);
	ccid->maxclock = иhgetl(p);
	p += sizeof(ccid->maxclock);

	ccid->nclocks = *p++;

	ccid->datarate = иhgetl(p);
	p += sizeof(ccid->datarate);
	ccid->maxdatarate = иhgetl(p);
	p += sizeof(ccid->maxdatarate);

	ccid->ndatarates = *p++;

	ccid->maxifsd = иhgetl(p);
	p += sizeof(ccid->maxifsd);
	ccid->synchproto = иhgetl(p);
	p += sizeof(ccid->synchproto);
	ccid->mechanical = иhgetl(p);
	p += sizeof(ccid->mechanical);
	ccid->features = иhgetl(p);
	p += sizeof(ccid->features);

	ccid->maxmsglen = иhgetl(p);
	p += sizeof(ccid->maxmsglen);
	ccid->classgetresp = *p++;
	ccid->classenvel = *p++;

	ccid->lcdlayout = иhgets(p);
	p += sizeof(ccid->lcdlayout);
	ccid->pinsupport = *p++;
	ccid->maxbusyslots = *p;

	return 0;
}


static int
isokdesc(Desc *d)
{
	DDesc *dd;

	dd = &d->data;
	if(dd->bLength == 0x36 && dd->bDescriptorType == 0x21)
		return 1;
	else
		return 0;
}


static int
ccidparsedesc(Ccid *ccid)
{
	int i;
	Usbdev *ud;
	Desc *d;

	ud = ccid->dev->usb;

	for(i = 0; i < nelem(ud->ddesc); i++)
		if((d = ud->ddesc[i]) != nil && isokdesc(d)){
			ccidgetdesc(ccid, &d->data);
			return 0;
		}
	return -1;
}

static int
usage(void)
{
	werrstr("usage: usb/ccid [-dkmn] [-a n]");
	return -1;
}

static void
cciddevfree(void *a)
{
	Ccid *ccid = a;

	if(ccid == nil)
		return;
	if(ccid->hasintr)
		closedev(ccid->epintr);
	closedev(ccid->epin);
	closedev(ccid->epout);
	ccid->epintr = ccid->epin = ccid->epout = nil;
	free(ccid);
}

static Usbfs ccidfs = {
	.walk =	dwalk,
	.open =	dopen,
	.read =	dread,
	.write=	dwrite,
	.stat =	dstat,
	.clunk=	dclunk,
};

/* called with the ccid lock */
int
ccidslotinit(Ccid *ccid, int nsl)
{
	Client *cl;
	Cmsg *c;
	char *s;
	uchar *atr;
	Atr *a;
	s = nil;
	int na;

	dcprint(2, "ccid: slot %d became active\n", nsl);

	if(nsl != 0){
		dcprint(2, "ccid: error nslots != 0 not supported: %d\n", nsl);
		return -1;
	}
	if(ccid->sl != nil){
		fprint(2, "ccid: bogus slot %d change act->act \n", nsl);
		return -1;
	}
	ccid->sl = emallocz(sizeof (Slot), 1);

		
	/* make sure it is off, the command may not be implemented 
	 * if the thing is not insertable (nfc reader)
	 */
	c = iccmsgrpc(ccid, nil, 0, IccPowerOffTyp, 0);
	if( c == nil )
		fprint(2, "ccid: error %r\n");
	free(c);

	c = iccmsgrpc(ccid, nil, 0, IccPowerOnTyp, 0);
	if( c == nil )
		goto Error;

	/* I got an ATR, I have a card!! */


	na = parseatr(&ccid->sl->patr, c->data, c->len);
	if(c->len != na){
		fprint(2, "ccid: bad atr, parsed len %lud, c->len %d\n", c->len, na);
		goto Error;
	}
	/* BUG, we use the default clock */
	fillparam(&ccid->sl->param, &ccid->sl->patr);

	s = lookupatr(c->data, c->len);
	atr = malloc(c->len);
	if(atr == nil)
		goto Error;
	
	a = listmatchatr(c->data, c->len);
	if(a != nil){	
		ccid->sl->Iccops = *(a->o);
	}
	else{
		ccid->sl->desc = s;
		ccid->sl->Iccops = defops;
		fprint(2, "ccid: generic smartcard %s\n", s);
	}
	memmove(atr, c->data, c->len);
	ccid->sl->natr = c->len;
	ccid->sl->atr = atr;
	cl = matchclient(ccid, c->data, c->len);
	if(cl != nil){
		cl->sl = ccid->sl;
		sendp(cl->start, nil);
	}

	ccid->sl->init(ccid);	/* this has to be even before powering.. unlock */
	ccid->sl->nreq++;	/* Status succeeded */

	free(c);
	c = iccmsgrpc(ccid, &ccid->sl->param, 5, SetParamTyp, 0);
	if( c == nil )
		goto Error;
	free(c);

	return 0;
Error:
	free(c);
	free(s);
	fprint(2, "error initializing slot\n");
	return -1;
}

/* called with the ccid lock BUG rpc midway? */
void
ccidslotfree(Ccid *ccid, int nsl)
{
	Slot *s;

	dcprint(2, "ccid: slot %d became inactive\n", nsl);
	s = ccid->sl;

	if(s == nil)
		fprint(2, "ccid: bogus state change on slot %d in->in\n", nsl);
	ccid->sl = nil;
	free(s->atr);
	free(s->desc);
	free(s);
}

static int
ccidinit(Ccid *ccid, int nsl)
{
	int iccpres;
	Cmsg *c;
	int res;

	res = 0;
	if(nsl != 0){
		fprint(2, "ccid: more than one slot, not implemented yet\n");
		return -1;
	}
	qlock(ccid);

	iccpres = isiccpresent(ccid);
	if(iccpres){
		ccidslotinit(ccid, nsl);
		if(ccid->sl == nil || ccid->sl->init == nil){
			res = -1;
				goto Error;
		}
	}
	else{
		/*
		 * BUG: can I look always present somewhere descriptor?
		 * some people report no Iccpresent, but if there is one, tickle it 
		*/
		dcprint(2, "ccid: nothing reported, trying to tickle\n");
		c = iccmsgrpc(ccid, nil, 0, IccPowerOffTyp, 0);
		if( c == nil )
			dcprint(2, "ccid: not really error %r\n");
		free(c);

		c = iccmsgrpc(ccid, nil, 0, IccPowerOnTyp, 0);
		if( c == nil ){
			dcprint(2, "ccid: not really error %r\n");
			res = 0;
			goto Error;	/* not really an error */
		}
		/* I got a response so there is something on 0 */
		ccidslotinit(ccid, nsl);
		if(ccid->sl == nil || ccid->sl->init == nil){
			res = -1;
			goto Error;
		}
		ccid->sl->init(ccid);
		free(c);
	}

Error:
	qunlock(ccid);	
	return res;
}


static void
ccidslotchange(Ccid *ccid, Imsg *im)
{
	int i;
	uchar sc;

	sc = im->sloticcstate;
	qlock(ccid);
	for(i = 0; i < 4; i++){
		if(sc & 0x2){	/* has changed */
			if(sc&0x1){
				ccidslotinit(ccid, i);
			}
			else
				ccidslotfree(ccid, i);
				
		}
		sc >>= 2;
	}
	qunlock(ccid);
}

static int
intignore(void *, char *msg)
{

	if(strcmp(msg, "alarm") == 0){
		return 1;
	}
	return 0;
}


int
ccidmain(Dev *dev, int argc, char* argv[])
{
	Ccid *ccid;
	char buf[64];

	ARGBEGIN{
	case 'd':
		cciddebug++;
		break;
	default:
		return usage();
	}ARGEND
	if(argc != 0)
		return usage();

	ccid = dev->aux = emallocz(sizeof(Ccid), 1);
	ccid->dev = dev;
	dev->free = cciddevfree;

	snprint(buf, sizeof buf, "vid %#06x did %#06x\n",
			dev->usb->vid, dev->usb->did);
	dcprint(2, "ccid: %63.63s\n", buf);
	ccidparsedesc(ccid);
	if(ccid->maxslot !=  0){
		fprint(2, "ccid: unimplemented multiple slots maxslot: %d",
			ccid->maxslot);
		return -1;
	}

	if(findendpoints(ccid) < 0){
		werrstr("ccid: endpoints not found");
		return -1;
	}

	ccid->fs = ccidfs;
	if(matchtagrd(buf) ==0)
		dprint(2, "ccid: ACS reader detected, delaying till ATR\n");
	if(matchscr3310(buf) ==0)
		dprint(2, "ccid: \"Belgium id reader detected\"\n");
		

	/* BUG only one slot */
	if(ccidinit(ccid, 0) < 0){
		dprint(2, "ccid: ccidinit: %r\n");
		return -1;
	}
	dprint(2, "ccid: slot[0] is active?: %p\n", ccid->sl);

	incref(dev);
	threadcreate(statusreader, ccid, Stacksz);

	snprint(ccid->fs.name, sizeof(ccid->fs.name), "ccidU%d", dev->id);
	ccid->fs.dev = dev;
	incref(dev);
	ccid->fs.aux = ccid;
	usbfsadd(&ccid->fs);
	closedev(dev);
	return 0;
}


