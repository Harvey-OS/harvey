/*
 * usb/disk - usb mass storage file server
 */
#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "scsireq.h"
#include "usb.h"

/*
 * mass storage transport protocols and subclasses,
 * from usb mass storage class specification overview rev 1.2
 */
enum {
	Protocbi =	0,	/* control/bulk/interrupt; mainly floppies */
	Protocb =	1,	/*   "  with no interrupt; mainly floppies */
	Protobulk =	0x50,	/* bulk only */

	Subrbc =	1,	/* reduced blk cmds */
	Subatapi =	2,	/* cd/dvd using sff-8020i or mmc-2 cmd blks */
	Subqic =	3,	/* QIC-157 tapes */
	Subufi =	4,	/* floppy */
	Sub8070 =	5,	/* removable media, atapi-like */
	Subscsi =	6,	/* scsi transparent cmd set */
	Subisd200 =	7,	/* ISD200 ATA */
	Subdev =	0xff,	/* use device's value */
};

enum {
	GET_MAX_LUN_T =	RD2H | Rclass | Rinterface,
	GET_MAX_LUN =	0xFE,
	UMS_RESET_T =	RH2D | Rclass | Rinterface,
	UMS_RESET =	0xFF,

	MaxIOsize	= 256*1024,	/* max. I/O size */
//	Maxlun		= 256,
	Maxlun		= 32,
};

#define PATH(type, n)	((type) | (n)<<8)
#define TYPE(path)	((uchar)(path))
#define NUM(path)	((uint)(path)>>8)

enum {
	Qdir = 0,
	Qctl,
	Qn,
	Qraw,
	Qdata,

	CMreset = 1,

	Pcmd = 0,
	Pdata,
	Pstatus,
};

static char *subclass[] = {
	"?",
	"rbc",
	"atapi",
	"qic tape",
	"ufi floppy",
	"8070 removable",
	"scsi transparent",
	"isd200 ata",
};

typedef struct Dirtab Dirtab;
struct Dirtab {
	char	*name;
	int	mode;
};
Dirtab dirtab[] = {
	".",	DMDIR|0555,
	"ctl",	0640,
	nil,	DMDIR|0750,
	"raw",	0640,
	"data",	0640,
};

Cmdtab cmdtab[] = {
	CMreset,	"reset",	1,
};

/* these are 600 bytes each; ScsiReq is not tiny */
typedef struct Umsc Umsc;
struct Umsc {
	ScsiReq;
	ulong	blocks;
	vlong	capacity;
	uchar 	rawcmd[10];
	uchar	phase;
	char	*inq;
};

typedef struct Ums Ums;
struct Ums {
	Umsc	*lun;
	uchar	maxlun;
	int	fd2;
	int	fd;
	int	setupfd;
	int	ctlfd;
	uchar	epin;
	uchar	epout;
	char	dev[64];
};

int exabyte, force6bytecmds, needmaxlun;
long starttime;
long maxiosize = MaxIOsize;

volatile int timedout;

char *owner;

static Ums ums;
static int freakout;		/* flag: if true, drive freaks out if reset */

extern int debug;

static void umsreset(Ums *umsc, int doinit);

/*
 * USB transparent SCSI devices
 */
typedef struct Cbw Cbw;			/* command block wrapper */
struct Cbw {
	char	signature[4];		/* "USBC" */
	long	tag;
	long	datalen;
	uchar	flags;
	uchar	lun;
	uchar	len;
	char	command[16];
};

typedef struct Csw Csw;			/* command status wrapper */
struct Csw {
	char	signature[4];		/* "USBS" */
	long	tag;
	long	dataresidue;
	uchar	status;
};

enum {
	CbwLen		= 31,
	CbwDataIn	= 0x80,
	CbwDataOut	= 0x00,
	CswLen		= 13,
	CswOk		= 0,
	CswFailed	= 1,
	CswPhaseErr	= 2,
};

int
statuscmd(int fd, int type, int req, int value, int index, char *data,
	int count)
{
	char *wp;

	wp = emalloc9p(count + 8);
	wp[0] = type;
	wp[1] = req;
	PUT2(wp + 2, value);
	PUT2(wp + 4, index);
	PUT2(wp + 6, count);
	if(data != nil)
		memmove(wp + 8, data, count);
	if(write(fd, wp, count + 8) != count + 8){
		fprint(2, "%s: statuscmd: %r\n", argv0);
		return -1;
	}
	return 0;
}

int
statusread(int fd, char *buf, int count)
{
	if(read(fd, buf, count) < 0){
		fprint(2, "%s: statusread: %r\n", argv0);
		return -1;
	}
	return 0;
}

void
getmaxlun(Ums *ums)
{
	uchar max;

	if(needmaxlun &&
	    statuscmd(ums->setupfd, GET_MAX_LUN_T, GET_MAX_LUN, 0, 0, nil, 0)
	    == 0 && statusread(ums->setupfd, (char *)&max, 1) == 0)
		fprint(2, "%s: maxlun %d\n", argv0, max);	// DEBUG
	else
		max = 0;
	ums->lun = mallocz((max + 1) * sizeof *ums->lun, 1);
	assert(ums->lun);
	ums->maxlun = max;
}

int
umsinit(Ums *ums, int epin, int epout)
{
	uchar data[8], i;
	char fin[128];
	Umsc *lun;

	if(ums->ctlfd == -1) {
		snprint(fin, sizeof fin, "%s/ctl", ums->dev);
		if((ums->ctlfd = open(fin, OWRITE)) == -1)
			return -1;
		if(epin == epout) {
			if(fprint(ums->ctlfd, "ep %d bulk rw 64 16", epin) < 0)
				return -1;
		} else {
			if(fprint(ums->ctlfd, "ep %d bulk r 64 16", epin) < 0 ||
			   fprint(ums->ctlfd, "ep %d bulk w 64 16", epout) < 0)
				return -1;
		}
		snprint(fin, sizeof fin, "%s/ep%ddata", ums->dev, epin);
		if((ums->fd = open(fin, OREAD)) == -1)
			return -1;
		snprint(fin, sizeof fin, "%s/ep%ddata", ums->dev, epout);
		if((ums->fd2 = open(fin, OWRITE)) == -1)
			return -1;
		snprint(fin, sizeof fin, "%s/setup", ums->dev);
		if ((ums->setupfd = open(fin, ORDWR)) == -1)
			return -1;
	}

	ums->epin = epin;
	ums->epout = epout;
	umsreset(ums, 0);
	getmaxlun(ums);
	for(i = 0; i <= ums->maxlun; i++) {
		lun = &ums->lun[i];
		lun->lun = i;
		lun->umsc = lun;			/* pointer to self */
		lun->flags = Fopen | Fusb | Frw10;
		if(SRinquiry(lun) == -1)
			return -1;
		lun->inq = smprint("%.48s", (char *)lun->inquiry+8);
		SRstart(lun, 1);
		if (SRrcapacity(lun, data) == -1 &&
		    SRrcapacity(lun, data) == -1) {
			lun->blocks = 0;
			lun->capacity = 0;
			lun->lbsize = 0;
		} else {
			lun->lbsize = data[4]<<24|data[5]<<16|data[6]<<8|data[7];
			lun->blocks = data[0]<<24|data[1]<<16|data[2]<<8|data[3];
			lun->blocks++; /* SRcapacity returns LBA of last block */
			lun->capacity = (vlong)lun->blocks * lun->lbsize;
		}
	}
	return 0;
}

static void
unstall(Ums *ums, int ep)
{
	if(fprint(ums->ctlfd, "unstall %d", ep & 0xF) < 0)
		fprint(2, "ctl write failed\n");
	if(fprint(ums->ctlfd, "data %d 0", ep & 0xF) < 0)
		fprint(2, "ctl write failed\n");

	statuscmd(ums->setupfd, RH2D | Rstandard | Rendpt, CLEAR_FEATURE, 0,
		0<<8 | ep, nil, 0);
}

static void
umsreset(Ums *umsc, int doinit)
{
	if (!freakout)
		statuscmd(umsc->setupfd, UMS_RESET_T, UMS_RESET, 0, 0, nil, 0);

	unstall(umsc, umsc->epin|0x80);
	unstall(umsc, umsc->epout);
	if(doinit && umsinit(&ums, umsc->epin, umsc->epout) < 0)
		sysfatal("device error");
}

long
umsrequest(Umsc *umsc, ScsiPtr *cmd, ScsiPtr *data, int *status)
{
	Cbw cbw;
	Csw csw;
	int n;
	static int seq = 0;

	memcpy(cbw.signature, "USBC", 4);
	cbw.tag = ++seq;
	cbw.datalen = data->count;
	cbw.flags = data->write? CbwDataOut: CbwDataIn;
	cbw.lun = umsc->lun;
	cbw.len = cmd->count;
	memcpy(cbw.command, cmd->p, cmd->count);
	memset(cbw.command + cmd->count, 0, sizeof(cbw.command) - cmd->count);

	if(debug) {
		fprint(2, "cmd:");
		for (n = 0; n < cbw.len; n++)
			fprint(2, " %2.2x", cbw.command[n]&0xFF);
		fprint(2, " datalen: %ld\n", cbw.datalen);
	}
	if(write(ums.fd2, &cbw, CbwLen) != CbwLen){
		fprint(2, "usbscsi: write cmd: %r\n");
		goto reset;
	}
	if(data->count != 0) {
		if(data->write)
			n = write(ums.fd2, data->p, data->count);
		else
			n = read(ums.fd, data->p, data->count);
		if(n == -1){
			if(debug)
				fprint(2, "usbscsi: data %sput: %r\n",
					data->write? "out": "in");
			if(data->write)
				unstall(&ums, ums.epout);
			else
				unstall(&ums, ums.epin | 0x80);
		}
	}
	n = read(ums.fd, &csw, CswLen);
	if(n == -1){
		unstall(&ums, ums.epin | 0x80);
		n = read(ums.fd, &csw, CswLen);
	}
	if(n != CswLen || strncmp(csw.signature, "USBS", 4) != 0){
		fprint(2, "usbscsi: read status: %r\n");
		goto reset;
	}
	if(csw.tag != cbw.tag) {
		fprint(2, "usbscsi: status tag mismatch\n");
		goto reset;
	}
	if(csw.status >= CswPhaseErr){
		fprint(2, "usbscsi: phase error\n");
		goto reset;
	}
	if(debug) {
		fprint(2, "status: %2.2ux residue: %ld\n",
			csw.status, csw.dataresidue);
		if(cbw.command[0] == ScmdRsense) {
			fprint(2, "sense data:");
			for (n = 0; n < data->count - csw.dataresidue; n++)
				fprint(2, " %2.2x", data->p[n]);
			fprint(2, "\n");
		}
	}

	if(csw.status == CswOk)
		*status = STok;
	else
		*status = STcheck;
	return data->count - csw.dataresidue;

reset:
	umsreset(&ums, 0);
	*status = STharderr;
	return -1;
}

int
findendpoints(Device *d, int *epin, int *epout)
{
	Endpt *ep;
	ulong csp;
	int i, addr, nendpt;

	*epin = *epout = -1;
	nendpt = 0;
	if(d->nconf < 1)
		return -1;
	for(i=0; i<d->nconf; i++) {
		if (d->config[i] == nil)
			d->config[i] = mallocz(sizeof(*d->config[i]),1);
		loadconfig(d, i);
	}
	for(i = 0; i < Nendpt; i++){
		if((ep = d->ep[i]) == nil)
			continue;
		nendpt++;
		csp = ep->csp;
		if(!(Class(csp) == CL_STORAGE && (Proto(csp) == 0x50)))
			continue;
		if(ep->type == Ebulk) {
			addr = ep->addr;
			if (debug)
				print("findendpoints: bulk; ep->addr %ux\n",
					ep->addr);
			if (ep->dir == Eboth || addr&0x80)
				if(*epin == -1)
					*epin =  addr&0xF;
			if (ep->dir == Eboth || !(addr&0x80))
				if(*epout == -1)
					*epout = addr&0xF;
		}
	}
	if(nendpt == 0) {
		if(*epin == -1)
			*epin = *epout;
		if(*epout == -1)
			*epout = *epin;
	}
	if (*epin == -1 || *epout == -1)
		return -1;
	return 0;
}

int
timeoutok(void)
{
	if (freakout)
		return 1;	/* OK; keep trying */
	else {
		fprint(2, "%s: no response from device.  unplug and replug "
			"it and try again with -f\n", argv0);
		return 0;	/* die */
	}
}

int
notifyf(void *, char *s)
{
	if(strcmp(s, "alarm") != 0)
		return 0;		/* die */
	if (!timeoutok()) {
		fprint(2, "%s: timed out\n", argv0);
		return 0;		/* die */
	}
	alarm(120*1000);
	fprint(2, "%s: resetting alarm\n", argv0);
	timedout = 1;
	return 1;			/* keep going */
}

int
devokay(int ctlrno, int id)
{
	int epin = -1, epout = -1;
	long time;
	Device *d;
	static int beenhere;

	if (!beenhere) {
		atnotify(notifyf, 1);
		beenhere = 1;
	}
	time = alarm(15*1000);
	d = opendev(ctlrno, id);
	if (describedevice(d) < 0) {
		perror("");
		closedev(d);
		alarm(time);
		return 0;
	}
	if (findendpoints(d, &epin, &epout) < 0) {
		fprint(2, "%s: bad usb configuration for ctlr %d id %d\n",
			argv0, ctlrno, id);
		closedev(d);
		alarm(time);
		return 0;
	}
	closedev(d);

	snprint(ums.dev, sizeof ums.dev, "/dev/usb%d/%d", ctlrno, id);
	if (umsinit(&ums, epin, epout) < 0) {
		alarm(time);
		fprint(2, "%s: initialisation: %r\n", argv0);
		return 0;
	}
	alarm(time);
	return 1;
}

static char *
subclname(int subcl)
{
	if ((unsigned)subcl < nelem(subclass))
		return subclass[subcl];
	return "**GOK**";		/* traditional */
}

static int
scanstatus(int ctlrno, int id)
{
	int winner;
	ulong csp;
	char *p, *hex;
	char buf[64];
	Biobuf *f;

	/* read port status file */
	sprint(buf, "/dev/usb%d/%d/status", ctlrno, id);
	f = Bopen(buf, OREAD);
	if (f == nil)
		sysfatal("can't open %s: %r", buf);
	if (debug)
		fprint(2, "\n%s: reading %s\n", argv0, buf);
	winner = 0;
	while (!winner && (p = Brdline(f, '\n')) != nil) {
		p[Blinelen(f)-1] = '\0';
		if (debug && *p == 'E')			/* Enabled? */
			fprint(2, "%s: %s\n", argv0, p);
		for (hex = p; *hex != '\0' && *hex != '0'; hex++)
			continue;
		csp = atol(hex);

		if (Class(csp) == CL_STORAGE && Proto(csp) == Protobulk) {
			if (0)
				fprint(2, "%s: /dev/usb%d/%d: bulk storage "
					"of subclass %s\n", argv0, ctlrno, id,
					subclname(Subclass(csp)));
			switch (Subclass(csp)) {
			case Subatapi:
			case Sub8070:
			case Subscsi:
				winner++;
				break;
			}
		}
	}
	Bterm(f);
	return winner;
}

static int
findums(int *ctlrp, int *idp)
{
	int ctlrno, id, winner, devfd, ctlrfd, nctlr, nport;
	char buf[64];
	Dir *ctlrs, *cdp, *ports, *pdp;

	*ctlrp = *idp = -1;
	winner = 0;

	/* walk controllers */
	devfd = open("/dev", OREAD);
	if (devfd < 0)
		sysfatal("can't open /dev: %r");
	nctlr = dirreadall(devfd, &ctlrs);
	if (nctlr < 0)
		sysfatal("can't read /dev: %r");
	for (cdp = ctlrs; nctlr-- > 0 && !winner; cdp++) {
		if (strncmp(cdp->name, "usb", 3) != 0)
			continue;
		ctlrno = atoi(cdp->name + 3);

		/* walk ports within a controller */
		snprint(buf, sizeof buf, "/dev/%s", cdp->name);
		ctlrfd = open(buf, OREAD);
		if (ctlrfd < 0)
			sysfatal("can't open %s: %r", buf);
		nport = dirreadall(ctlrfd, &ports);
		if (nport < 0)
			sysfatal("can't read %s: %r", buf);
		for (pdp = ports; nport-- > 0 && !winner; pdp++) {
			if (!isdigit(*pdp->name))
				continue;
			id = atoi(pdp->name);

			/* read port status file */
			winner = scanstatus(ctlrno, id);
			if (winner)
				if (devokay(ctlrno, id)) {
					*ctlrp = ctlrno;
					*idp = id;
				} else
					winner = 0;
		}
		free(ports);
		close(ctlrfd);
	}
	free(ctlrs);
	close(devfd);
	if (!winner)
		return -1;
	else
		return 0;
}

void
rattach(Req *r)
{
	r->ofcall.qid.path = PATH(Qdir, 0);
	r->ofcall.qid.type = dirtab[Qdir].mode >> 24;
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

char*
rwalk1(Fid *fid, char *name, Qid *qid)
{
	int i, n;
	char buf[32];
	ulong path;

	path = fid->qid.path;
	if(!(fid->qid.type & QTDIR))
		return "walk in non-directory";

	if(strcmp(name, "..") == 0)
		switch(TYPE(path)) {
		case Qn:
			qid->path = PATH(Qn, NUM(path));
			qid->type = dirtab[Qn].mode >> 24;
			return nil;
		case Qdir:
			return nil;
		default:
			return "bug in rwalk1";
		}

	for(i = TYPE(path)+1; i < nelem(dirtab); i++) {
		if(i==Qn){
			n = atoi(name);
			snprint(buf, sizeof buf, "%d", n);
			if(n <= ums.maxlun && strcmp(buf, name) == 0){
				qid->path = PATH(i, n);
				qid->type = dirtab[i].mode>>24;
				return nil;
			}
			break;
		}
		if(strcmp(name, dirtab[i].name) == 0) {
			qid->path = PATH(i, NUM(path));
			qid->type = dirtab[i].mode >> 24;
			return nil;
		}
		if(dirtab[i].mode & DMDIR)
			break;
	}
	return "directory entry not found";
}

void
dostat(int path, Dir *d)
{
	Dirtab *t;

	memset(d, 0, sizeof(*d));
	d->uid = estrdup9p(owner);
	d->gid = estrdup9p(owner);
	d->qid.path = path;
	d->atime = d->mtime = starttime;
	t = &dirtab[TYPE(path)];
	if(t->name)
		d->name = estrdup9p(t->name);
	else {
		d->name = smprint("%ud", NUM(path));
		if(d->name == nil)
			sysfatal("out of memory");
	}
	if(TYPE(path) == Qdata)
		d->length = ums.lun[NUM(path)].capacity;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
}

static int
dirgen(int i, Dir *d, void*)
{
	i += Qdir + 1;
	if(i <= Qn) {
		dostat(i, d);
		return 0;
	}
	i -= Qn;
	if(i <= ums.maxlun) {
		dostat(PATH(Qn, i), d);
		return 0;
	}
	return -1;
}

static int
lungen(int i, Dir *d, void *aux)
{
	int *c;

	c = aux;
	i += Qn + 1;
	if(i <= Qdata){
		dostat(PATH(i, NUM(*c)), d);
		return 0;
	}
	return -1;
}

void
rstat(Req *r)
{
	dostat((long)r->fid->qid.path, &r->d);
	respond(r, nil);
}

void
ropen(Req *r)
{
	ulong path;

	path = r->fid->qid.path;
	switch(TYPE(path)) {
	case Qraw:
		ums.lun[NUM(path)].phase = Pcmd;
		break;
	}
	respond(r, nil);
}

void
rread(Req *r)
{
	int bno, nb, len, offset, n;
	ulong path;
	uchar i;
	char buf[8192], *p;
	Umsc *lun;

	path = r->fid->qid.path;
	switch(TYPE(path)) {
	case Qdir:
		dirread9p(r, dirgen, 0);
		break;
	case Qn:
		dirread9p(r, lungen, &path);
		break;
	case Qctl:
		n = 0;
		for(i = 0; i <= ums.maxlun; i++) {
			lun = &ums.lun[i];
			n += snprint(buf + n, sizeof buf - n, "%d: ", i);
			if(lun->flags & Finqok)
				n += snprint(buf + n, sizeof buf - n,
					"inquiry %s ", lun->inq);
			if(lun->blocks > 0)
				n += snprint(buf + n, sizeof buf - n,
					"geometry %ld %ld", lun->blocks,
					lun->lbsize);
			n += snprint(buf + n, sizeof buf - n, "\n");
		}
		readbuf(r, buf, n);
		break;
	case Qraw:
		lun = &ums.lun[NUM(path)];
		if(lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		switch(lun->phase) {
		case Pcmd:
			respond(r, "phase error");
			return;
		case Pdata:
			lun->data.p = (uchar*)r->ofcall.data;
			lun->data.count = r->ifcall.count;
			lun->data.write = 0;
			n = umsrequest(lun, &lun->cmd, &lun->data, &lun->status);
			lun->phase = Pstatus;
			if (n == -1) {
				respond(r, "IO error");
				return;
			}
			r->ofcall.count = n;
			break;
		case Pstatus:
			n = snprint(buf, sizeof buf, "%11.0ud ", lun->status);
			if (r->ifcall.count < n)
				n = r->ifcall.count;
			memmove(r->ofcall.data, buf, n);
			r->ofcall.count = n;
			lun->phase = Pcmd;
			break;
		}
		break;
	case Qdata:
		lun = &ums.lun[NUM(path)];
		if(lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		bno = r->ifcall.offset / lun->lbsize;
		nb = (r->ifcall.offset + r->ifcall.count + lun->lbsize - 1)
			/ lun->lbsize - bno;
		if(bno + nb > lun->blocks)
			nb = lun->blocks - bno;
		if(bno >= lun->blocks || nb == 0) {
			r->ofcall.count = 0;
			break;
		}
		if(nb * lun->lbsize > maxiosize)
			nb = maxiosize / lun->lbsize;
		p = malloc(nb * lun->lbsize);
		if (p == 0) {
			respond(r, "no mem");
			return;
		}
		lun->offset = r->ifcall.offset / lun->lbsize;
		n = SRread(lun, p, nb * lun->lbsize);
		if(n == -1) {
			free(p);
			respond(r, "IO error");
			return;
		}
		len = r->ifcall.count;
		offset = r->ifcall.offset % lun->lbsize;
		if(offset + len > n)
			len = n - offset;
		r->ofcall.count = len;
		memmove(r->ofcall.data, p + offset, len);
		free(p);
		break;
	}
	respond(r, nil);
}

void
rwrite(Req *r)
{
	int n, bno, nb, len, offset;
	ulong path;
	char *p;
	Cmdbuf *cb;
	Cmdtab *ct;
	Umsc *lun;

	n = r->ifcall.count;
	r->ofcall.count = 0;
	path = r->fid->qid.path;
	switch(TYPE(path)) {
	case Qctl:
		cb = parsecmd(r->ifcall.data, n);
		ct = lookupcmd(cb, cmdtab, nelem(cmdtab));
		if(ct == 0) {
			respondcmderror(r, cb, "%r");
			return;
		}
		switch(ct->index) {
		case CMreset:
			umsreset(&ums, 1);
		}
		break;
	case Qraw:
		lun = &ums.lun[NUM(path)];
		if(lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		n = r->ifcall.count;
		switch(lun->phase) {
		case Pcmd:
			if(n != 6 && n != 10) {
				respond(r, "bad command length");
				return;
			}
			memmove(lun->rawcmd, r->ifcall.data, n);
			lun->cmd.p = lun->rawcmd;
			lun->cmd.count = n;
			lun->cmd.write = 1;
			lun->phase = Pdata;
			break;
		case Pdata:
			lun->data.p = (uchar*)r->ifcall.data;
			lun->data.count = n;
			lun->data.write = 1;
			n = umsrequest(lun, &lun->cmd, &lun->data, &lun->status);
			lun->phase = Pstatus;
			if(n == -1) {
				respond(r, "IO error");
				return;
			}
			break;
		case Pstatus:
			lun->phase = Pcmd;
			respond(r, "phase error");
			return;
		}
		break;
	case Qdata:
		lun = &ums.lun[NUM(path)];
		if(lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		bno = r->ifcall.offset / lun->lbsize;
		nb = (r->ifcall.offset + r->ifcall.count + lun->lbsize-1)
			/ lun->lbsize - bno;
		if(bno + nb > lun->blocks)
			nb = lun->blocks - bno;
		if(bno >= lun->blocks || nb == 0) {
			r->ofcall.count = 0;
			break;
		}
		if(nb * lun->lbsize > maxiosize)
			nb = maxiosize / lun->lbsize;
		p = malloc(nb * lun->lbsize);
		if(p == 0) {
			respond(r, "no mem");
			return;
		}
		offset = r->ifcall.offset % lun->lbsize;
		len = r->ifcall.count;
		if(offset || (len % lun->lbsize) != 0) {
			lun->offset = r->ifcall.offset / lun->lbsize;
			n = SRread(lun, p, nb * lun->lbsize);
			if(n == -1) {
				free(p);
				respond(r, "IO error");
				return;
			}
			if(offset + len > n)
				len = n - offset;
		}
		memmove(p+offset, r->ifcall.data, len);
		lun->offset = r->ifcall.offset / lun->lbsize;
		n = SRwrite(lun, p, nb * lun->lbsize);
		if(n == -1) {
			free(p);
			respond(r, "IO error");
			return;
		}
		if(offset+len > n)
			len = n - offset;
		r->ofcall.count = len;
		free(p);
		break;
	}
	r->ofcall.count = n;
	respond(r, nil);
}

Srv usbssrv = {
	.attach = rattach,
	.walk1 = rwalk1,
	.open =	 ropen,
	.read =	 rread,
	.write = rwrite,
	.stat =	 rstat,
};

void (*dprinter[])(Device *, int, ulong, void *b, int n) = {
	[STRING] pstring,
	[DEVICE] pdevice,
};

void
usage(void)
{
	fprint(2, "usage: %s [-Ddfl] [-m mountpoint] [-s srvname] [ctrno id]\n",
		argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int ctlrno, id;
	char *srvname, *mntname;

	mntname = "/n/disk";
	srvname = nil;
	ctlrno = 0;
	id = 1;

	ARGBEGIN{
	case 'd':
		debug = Dbginfo;
		break;
	case 'f':
		freakout++;
		break;
	case 'l':
		needmaxlun++;
		break;
	case 'm':
		mntname = EARGF(usage());
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	case 'D':
		++chatty9p;
		break;
	default:
		usage();
	}ARGEND

	ums.ctlfd = ums.setupfd = ums.fd = ums.fd2 = -1;
	ums.maxlun = -1;
	if (argc == 0) {
		if (findums(&ctlrno, &id) < 0) {
			sleep(5*1000);
			if (findums(&ctlrno, &id) < 0)
				sysfatal("no usb mass storage device found");
		}
	} else if (argc == 2 && isdigit(argv[0][0]) && isdigit(argv[1][0])) {
		ctlrno = atoi(argv[0]);
		id = atoi(argv[1]);
		if (!devokay(ctlrno, id))
			sysfatal("no usable usb mass storage device at %d/%d",
				ctlrno, id);
	} else
		usage();

	starttime = time(0);
	owner = getuser();

	postmountsrv(&usbssrv, srvname, mntname, 0);
	exits(0);
}
