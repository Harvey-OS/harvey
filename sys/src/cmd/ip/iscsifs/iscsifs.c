#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <pool.h>
#include <9p.h>
#include <ip.h>
#include "scsireq.h"
#include "iscsi.h"

#define	ROUNDUP(s, sz)	(((s) + ((sz)-1)) & ~((sz)-1))

#define PATH(type, n)	((type) | (n)<<8)
#define TYPE(path)	((path) & 0xFF)
#define NUM(path)	((uint)(path)>>8)

enum {
	Qdir = 0,
	Qctl,
	Qn,
	Qraw,
	Qcctl,
	Qdata,

	CMreset = 1,

	Pcmd = 0,
	Pdata,
	Pstatus,

	Cnop = 0,
	Cscsi,
	Cmgmt,
	Clogin,
	Ctext,
	Cdataout,
	Clogout,

	Csnack = 0x10,

	Rnop = 0,
	Rscsi,
	Rmgmt,
	Rlogin,
	Rtext,
	Rdatain,
	Rlogout,

	Rr2t = 0x31,
	Rasync,

	Rreject = 0x3F,

//	Maxluns = 16,		/* realistic upper bound */
	Maxluns = 256 + 4,	/* iscsi targets can be weird */
	MaxPacketSize = 64*1024,
};

typedef struct Dirtab Dirtab;
typedef struct Ihdr Ihdr;
typedef struct Iscsi Iscsi;
typedef struct Iscsipkt Iscsipkt;
typedef struct Iscsis Iscsis;

struct Dirtab {
	char	*name;
	int	mode;
};
Dirtab dirtab[] = {
	".", 	DMDIR | 0555,
	"ctl", 	0640,
	nil, 	DMDIR | 0750,
	"raw", 	0640,
	"ctl", 	0640,
	"data",	0640,
};

Cmdtab cmdtab[] = {
	CMreset, 	"reset", 	1,
};

struct Iscsi {
	ScsiReq;
	ulong	blocks;
	vlong	capacity;
	uchar 	rawcmd[10];
	uchar	phase;
};

struct Iscsis {
	Iscsi	lun[Maxluns]; /* heavy handed; there will usually be 001 luns */
//	uchar	maxlun;
	uvlong	maxlun;
	int	fd;
};

struct Ihdr {
	uchar	op;
	uchar	flags;
	uchar	spec1;
	uchar	spec2;
	uchar	ahslen;
	uchar	datalen[3];
	uchar	lun[8];
	uchar	itt[4];
	uchar	ttt[4];
	uchar	statsn[4];
	uchar	expect[4];
	uchar	max[4];
	uchar	datasn[4];
	uchar	offset[4];
	uchar	rcount[4];
};

struct Iscsipkt {
	char	*buf;
	Ihdr	*hdr;
	long	len;
	char	*data;
};

Iscsis iscsi;
long starttime;
char *owner, *host, *vol;
int debug;

static char validlun[Maxluns + 1];

ulong
getbe2(uchar *p)
{
	return p[0]<<8 | p[1];
}

ulong
getbe3(uchar *p)
{
	return p[0]<<16 | p[1]<<8 | p[2];
}

ulong
getbe4(uchar *p)
{
	return p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3];
}

uvlong
getbe8(uchar *p)
{
	return (uvlong)getbe4(p) << 32 | getbe4(p+4);
}

void
putbe2(uchar *p, ushort us)
{
	*p++ = us >> 8;
	*p   = us;
}

void
putbe3(uchar *p, ulong ul)
{
	*p++ = ul >> 16;
	*p++ = ul >> 8;
	*p   = ul;
}

void
putbe4(uchar *p, ulong ul)
{
	*p++ = ul >> 24;
	*p++ = ul >> 16;
	*p++ = ul >> 8;
	*p   = ul;
}

void
freepkt(Iscsipkt *pkt)
{
	if (pkt != nil) {
		if (pkt->buf != nil)
			free(pkt->buf);
		free(pkt);
	}
}

void
dump(void *v, int bytes)
{
	uchar *p;

	if (!debug)
		return;
	p = v;
	while (bytes-- > 0)
		fprint(2, " %2.2x", *p++);
	fprint(2, "\n");
}

void
dumptext(char *tag, char *text, int len)
{
	int plen, left;
	char *p;

	if (!debug)
		return;
	for (p = text; len > 0; p += plen + 1, len -= plen + 1) {
		/* paranoia: last pair might not be NUL-terminated */
		plen = 0;
		for (left = len; left > 0 && p[plen] != '\0'; --left)
			plen++;
		fprint(2, "%s text `%.*s'\n", tag, plen, p);
	}
}

void
dumpresppkt(Iscsicmdresp *resp)
{
	if (!debug)
		return;
	fprint(2, "resp pkt: op %#x opspfc %#x %#x %#x dseglen %ld\n",
		resp->op, resp->opspfc[0], resp->opspfc[1], resp->opspfc[2],
		getbe3(resp->dseglen));
	fprint(2, "\titt %ld sts seq %ld exp cmd seq %ld\n", getbe4(resp->itt),
		getbe4(resp->stsseq), getbe4(resp->expcmdseq));
}

Iscsipkt *
mkpkt(int len)
{
	Iscsipkt *pkt;

	pkt = emalloc9p(sizeof(Iscsipkt));
	pkt->buf = emalloc9p(Bhdrsz + len);
	pkt->hdr = (Ihdr *)pkt->buf;
	pkt->data = pkt->buf + Bhdrsz;
	pkt->len = 0;
	return pkt;
}

Iscsipkt *
readpkt(int fd)
{
	ulong len;
	uchar ahslen;
	Iscsipkt *pkt;

	pkt = mkpkt(0);
	if (readn(fd, pkt->buf, Bhdrsz) != Bhdrsz)
		sysfatal("readn: %r");

	/* obscure way to get dseglen into pkt->len */
	ahslen = pkt->hdr->ahslen;
	pkt->hdr->ahslen = 0;
	pkt->len = (long)nhgetl(&pkt->hdr->ahslen);
	pkt->hdr->ahslen = ahslen;

	len = pkt->len;
	if (len != 0) {			/* data segment? */
		len = ROUNDUP(len, 4);
		if (debug)
			fprint(2, "added %ld data seg bytes to read\n", len);
		pkt->buf = erealloc9p(pkt->buf, Bhdrsz + len);
		pkt->hdr = (Ihdr *)pkt->buf;
		pkt->data = pkt->buf + Bhdrsz;

		if (readn(fd, pkt->data, len) != len)
			sysfatal("readn: %r");
	}
	return pkt;
}

char *
addpair(char *buf, char *name, char *val)
{
	strcpy(buf, name);
	buf += strlen(name);
	*buf++ = '=';
	strcpy(buf, val);
	buf += strlen(val);
	*buf++ = '\0';
	return buf;
}

int
iscsilogin(Iscsis *iscsi)
{
	Iscsipkt *pkt;
	char *attr;
	long len;

	pkt = mkpkt(MaxPacketSize - Bhdrsz);	/* wow, overkill */
	memset(pkt->buf, 0, MaxPacketSize);
	pkt->hdr->op = Clogin | Immed;
	pkt->hdr->flags = 0x87;			/* T, CSG=1, NSG=3 */
	pkt->hdr->lun[1] = 2;			/* actually isid: oui format */
	pkt->hdr->lun[2] = 0x3D;		/* 00023d is cisco */
	pkt->hdr->itt[1] = 0x0A;

	attr = pkt->data;
	attr = addpair(attr, "InitiatorName", "iqn.2012-12.com.bell-labs.plan9");
	attr = addpair(attr, "InitiatorAlias", "Plan 9");
	attr = addpair(attr, "TargetName", vol);
	attr = addpair(attr, "SessionType", "Normal");
	attr = addpair(attr, "HeaderDigest", "None");
	attr = addpair(attr, "DataDigest", "None");

	attr = addpair(attr, "DataPDUInOrder", "Yes");
	attr = addpair(attr, "DataSequenceInOrder", "Yes");
	attr = addpair(attr, "DefaultTime2Retain", "0");
	attr = addpair(attr, "DefaultTime2Wait", "0");
	attr = addpair(attr, "ErrorRecoveryLevel", "0");
	attr = addpair(attr, "FirstBurstLength", "262144");
	attr = addpair(attr, "IFMarker", "No");
	attr = addpair(attr, "ImmediateData", "Yes");
	attr = addpair(attr, "InitialR2T", "No");
	attr = addpair(attr, "MaxBurstLength", "16776192");
	attr = addpair(attr, "MaxConnections", "1");
	attr = addpair(attr, "MaxOutstandingR2T", "1");
	attr = addpair(attr, "MaxRecvDataSegmentLength", "131072");
	attr = addpair(attr, "OFMarker", "No");

	pkt->len = attr - pkt->data;
	putbe3(pkt->hdr->datalen, pkt->len);	/* dseglen actually */

	len = ROUNDUP(attr - pkt->buf, 4);
	if (debug)
		fprint(2, "%s: sending login pkt of %ld bytes\n", argv0, len);
	if (write(iscsi->fd, pkt->buf, len) != len)
		sysfatal("write: %r");
	freepkt(pkt);

	pkt = readpkt(iscsi->fd);
	if (pkt->hdr->datasn[0] != 0 || pkt->hdr->datasn[1] != 0) {
		fprint(2, "%s: non-zero login status %d %d\n", argv0,
			pkt->hdr->datasn[0], pkt->hdr->datasn[1]);
		dump(pkt->hdr, Bhdrsz);
		freepkt(pkt);
		return -1;
	}
	freepkt(pkt);
	return 0;
}

int
iscsiconnect(Iscsis *iscsi)
{
	iscsi->fd = dial(netmkaddr(host, "tcp", "3260"), 0, 0, 0);
	if (iscsi->fd < 0)
		sysfatal("dial: %r");
	if (iscsilogin(iscsi))
		sysfatal("login failed; check target name");
	return 0;
}

int
iscsiinit(Iscsis *iscsi)
{
	int gotluns, highlun;
	uint maxlun;
	uvlong luns[512];
	uchar data[8], i;
	Iscsi *lun;

	iscsi->maxlun = 0;
	lun = &iscsi->lun[0];
	lun->iscsi = lun;
	lun->flags = Fopen | Frw10;
	memset(luns, 0, sizeof luns);

	gotluns = 0;
	highlun = 0;
	maxlun = SRreportlun(lun, (uchar *)luns, sizeof luns);
	if ((int)maxlun < 0)
		maxlun = 8*8;		/* try first 8 luns */
	else
		gotluns = 1;

	iscsi->maxlun = maxlun / 8;
	if (debug)
		fprint(2, "maxlun %lld from reportlun\n", iscsi->maxlun);
	assert(iscsi->maxlun < Maxluns);

	for (i = 0; i < iscsi->maxlun; i++) {
		lun = &iscsi->lun[i];
		if (gotluns)
			lun->lun = luns[i + 1];
		else
			lun->lun = i;
		lun->iscsi = lun;
		lun->flags = Fopen | Frw10;
		lun->blocks = 0;
		lun->capacity = 0;
		lun->lbsize = 0;
		if (SRinquiry(lun) == -1) {
			lun->flags = lun->lun = 0;
			continue;
		}
		assert(lun->lun >= 0 && lun->lun < Maxluns);
		validlun[i] = 1;
		if (lun->lun > highlun)
			highlun = lun->lun;
		if (SRrcapacity(lun, data) != -1 ||
		    SRrcapacity(lun, data) != -1) {
			lun->lbsize = data[4] << 28 | data[5] << 16 |
				data[6] << 8 | data[7];
			lun->blocks = data[0] << 24 | data[1] << 16 |
				data[2] << 8 | data[3];
			lun->blocks++;		/* last block # -> size */
			lun->capacity = (vlong)lun->blocks * lun->lbsize;
		}
	}
	iscsi->maxlun = highlun + 1;
	if (debug)
		fprint(2, "actual maxlun %lld\n", iscsi->maxlun);
	assert(iscsi->maxlun < Maxluns);
	return 0;
}

void
iscsireset(Iscsis *iscsi)
{
	if (iscsiinit(iscsi) < 0) {
		iscsiconnect(iscsi);
		if (iscsiinit(iscsi) < 0)
			sysfatal("iscsireset failed");
	}
}

long
iscsirequest(Iscsi *lun, ScsiPtr *cmd, ScsiPtr *data, int *status)
{
	int n, scmd;
	uint bytes, dseglen;
	long rcount;
	Iscsipkt *pkt, *rpkt;
	static int seq = -1;

	/* format iscsi command request packet */
	rcount = 0;
	pkt = mkpkt(MaxPacketSize - Bhdrsz);	/* wow, overkill */
	memset(pkt->hdr, 0, sizeof(Ihdr));
	pkt->hdr->op = Iopcmd;
	pkt->hdr->flags = 0x81;
	if (data->count != 0)
		if (data->write)
			pkt->hdr->flags |= 0x20;
		else
			pkt->hdr->flags |= 0x40;

	memmove(pkt->hdr->lun, &lun->lun, 4);
	hnputl(pkt->hdr->itt, ++seq);
	hnputl(pkt->hdr->ttt, data->count);
	hnputl(pkt->hdr->statsn, seq);
	hnputl(pkt->hdr->expect, seq);

	/* deal with any outgoing data segment */
	assert(cmd->count >= 0 && cmd->count <= 16);
	memmove(pkt->hdr->max, cmd->p, cmd->count);
	if (cmd->count < 16)
		memset(pkt->hdr->max + cmd->count, 0, 16 - cmd->count);
	if (data->write != 0) {
		memmove(pkt->data, data->p, data->count);
		pkt->len = ROUNDUP(data->count, 4);
		if (pkt->len != data->count)
			memset(pkt->data + data->count, 0,
				pkt->len - data->count);
	} else
		pkt->len = 0;
	hnputl(&pkt->hdr->ahslen, pkt->len);	/* dseglen actually */
	pkt->hdr->ahslen = 0;

	if (debug) {
		fprint(2, "cmd:");
		for (n = 0; n < 16; n++)
			fprint(2, " %02.2#ux", *(pkt->hdr->max + n) & 0xFF);
		fprint(2, " datalen: %ld\n", pkt->len);
	}

	/* send the iscsi request packet */
	if (write(iscsi.fd, pkt->buf, pkt->len + Bhdrsz) != pkt->len + Bhdrsz)
		sysfatal("write: %r");
	scmd = pkt->hdr->max[0];
	freepkt(pkt);

	/* await the iscsi response packet */
	rpkt = readpkt(iscsi.fd);

	/* deal with any incoming data segment */
	switch (rpkt->hdr->op) {
	case Topresp:
	case Topdatain:
		bytes = data->count;			/* max we can return */
		dseglen = getbe3(rpkt->hdr->datalen);	/* how much we have */
		if (bytes > dseglen)
			bytes = dseglen;	/* only return what we have */

		rcount = nhgetl(rpkt->hdr->rcount);
		if (debug)
			fprint(2, "residue: %ld\n", rcount);
		if (data->count != 0 && data->write == 0)
			memmove(data->p, rpkt->data, bytes);
		if (debug)
			if (scmd == ScmdRsense) {
				fprint(2, "sense data:");
				for (n = 0; n < bytes; n++)
					fprint(2, " %2.2x", data->p[n] & 0xFF);
				fprint(2, "\n");
			}

		if (scmd == ScmdReportlun) {
			data->count = nhgetl(rpkt->data);
			rcount = 0;
		}

		/* is there a following iscsi response packet? */
		if (rpkt->hdr->op == Topdatain && !(rpkt->hdr->flags & 1)) {
			freepkt(rpkt);
			rpkt = readpkt(iscsi.fd);	/* ignore it */
		}
		break;
	}

	if (debug) {
		fprint(2, "hdr:");
		for (n = 0; n < Bhdrsz; n++)
			fprint(2, " %02.2#ux", *(rpkt->buf + n) & 0xFF);
		fprint(2, "\n");
	}

	if (rpkt->hdr->op != Topresp && rpkt->hdr->op != Topdatain) {
		*status = STharderr;
		freepkt(rpkt);
		return -1;
	}
	if (debug)
		fprint(2, "status: %02.2#ux\n", rpkt->hdr->spec2);

	if (rpkt->hdr->spec2 == 0)
		*status = STok;
	else
		*status = STcheck;
	freepkt(rpkt);
	return data->count - rcount;
}


/*
 * 9P
 */

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
	ulong path;
	char buf[32];

	path = fid->qid.path;
	if (!(fid->qid.type & QTDIR))
		return "walk in non-directory";

	if (strcmp(name, "..") == 0)
		switch (TYPE(path)) {
		case Qn:
			qid->path = PATH(Qn, NUM(path));
			qid->type = dirtab[Qn].mode >> 24;
			return nil;
		case Qdir:
			return nil;
		default:
			return "bug in rwalk1";
		}

	i = TYPE(path) + 1;
	n = atoi(name);
	snprint(buf, sizeof buf, "%d", n);
	for (; i < nelem(dirtab); i++) {
		if (i == Qn) {
			if (n <= iscsi.maxlun && validlun[n] &&
			    strcmp(buf, name) == 0) {
				qid->path = PATH(i, n);
				qid->type = dirtab[i].mode >> 24;
				return nil;
			}
			break;
		}
		if (strcmp(name, dirtab[i].name) == 0) {
			qid->path = PATH(i, NUM(path));
			qid->type = dirtab[i].mode >> 24;
			return nil;
		}
		if (dirtab[i].mode & DMDIR)
			break;
	}
	return "directory entry not found";
}

void
dostat(int path, Dir *d)
{
	Dirtab * t;

	memset(d, 0, sizeof(*d));
	d->uid = estrdup9p(owner);
	d->gid = estrdup9p(owner);
	d->qid.path = path;
	d->atime = d->mtime = starttime;
	t = &dirtab[TYPE(path)];
	if (t->name)
		d->name = estrdup9p(t->name);
	else {
		d->name = smprint("%ud", NUM(path));
		if (d->name == nil)
			sysfatal("out of memory");
	}
	if (TYPE(path) == Qdata)
		d->length = iscsi.lun[NUM(path)].capacity;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
}

static int
dirgen(int i, Dir *d, void*)
{
	i += Qdir + 1;
	if (i <= Qn) {
		dostat(i, d);
		return 0;
	}
	i -= Qn;
	if (i < iscsi.maxlun && validlun[i]) {
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
	if (i <= Qdata) {
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
	switch (TYPE(path)) {
	case Qraw:
		iscsi.lun[NUM(path)].phase = Pcmd;
		break;
	}
	respond(r, nil);
}

void
rread(Req *r)
{
	int bno, nb, len, offset, n;
	ulong path;
	char buf[8192], *p;
	uchar i;
	Iscsi *lun;

	path = r->fid->qid.path;
	lun = &iscsi.lun[NUM(path)];		/* default, common case */
	switch (TYPE(path)) {
	case Qdir:
		dirread9p(r, dirgen, 0);
		break;
	case Qn:
		dirread9p(r, lungen, &path);
		break;
	case Qctl:
		n = 0;
		for (i = 0; i < iscsi.maxlun; i++) {
			if (!validlun[i])
				continue;
			lun = &iscsi.lun[i];
			n += snprint(buf + n, sizeof buf - n, "%d: ", i);
			if (lun->flags & Finqok)
				n += snprint(buf + n, sizeof buf - n,
					"inquiry %.48s\n",
					(char *)lun->inquiry + 8);
			if (lun->blocks > 0)
				n += snprint(buf + n, sizeof buf - n,
					"geometry %ld %ld\n", lun->blocks,
					lun->lbsize);
		}
		readbuf(r, buf, n);
		break;
	case Qcctl:
		if (lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		n = snprint(buf, sizeof buf, "inquiry %.48s\n",
			(char *)lun->inquiry + 8);
		if (lun->blocks > 0)
			n += snprint(buf + n, sizeof buf - n,
				"geometry %ld %ld\n", lun->blocks, lun->lbsize);
		readbuf(r, buf, n);
		break;
	case Qraw:
		if (lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		switch (lun->phase) {
		case Pcmd:
			respond(r, "phase error");
			return;
		case Pdata:
			lun->data.p = (uchar *)r->ofcall.data;
			lun->data.count = r->ifcall.count;
			lun->data.write = 0;
			n = iscsirequest(lun, &lun->cmd, &lun->data,
				&lun->status);
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
		if (lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		bno = r->ifcall.offset / lun->lbsize;
		nb = (r->ifcall.offset + r->ifcall.count + lun->lbsize - 1) /
			lun->lbsize - bno;
		if (bno + nb > lun->blocks)
			nb = lun->blocks - bno;
		if (bno >= lun->blocks || nb == 0) {
			r->ofcall.count = 0;
			break;
		}
		if (nb * lun->lbsize > MaxIOsize)
			nb = MaxIOsize / lun->lbsize;
		p = malloc(nb * lun->lbsize);
		if (p == 0) {
			respond(r, "no memory");
			return;
		}
		lun->offset = r->ifcall.offset / lun->lbsize;
		n = SRread(lun, p, nb * lun->lbsize);
		if (n == -1) {
			free(p);
			respond(r, "IO error");
			return;
		}
		len = r->ifcall.count;
		offset = r->ifcall.offset % lun->lbsize;
		if (offset + len > n)
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
	int bno, nb, len, offset, n;
	ulong path;
	char *p;
	Cmdbuf *cb;
	Cmdtab *ct;
	Iscsi *lun;

	n = r->ifcall.count;
	r->ofcall.count = 0;
	path = r->fid->qid.path;
	lun = &iscsi.lun[NUM(path)];		/* default, common case */
	switch (TYPE(path)) {
	case Qctl:
		cb = parsecmd(r->ifcall.data, n);
		ct = lookupcmd(cb, cmdtab, nelem(cmdtab));
		if (ct == 0) {
			respondcmderror(r, cb, "%r");
			return;
		}
		switch (ct->index) {
		case CMreset:
			iscsireset(&iscsi);
			break;
		}
		break;
	case Qraw:
		if (lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		n = r->ifcall.count;
		switch (lun->phase) {
		case Pcmd:
			if (n != 6 && n != 10) {
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
			lun->data.p = (uchar *)r->ifcall.data;
			lun->data.count = n;
			lun->data.write = 1;
			n = iscsirequest(lun, &lun->cmd, &lun->data,
				&lun->status);
			lun->phase = Pstatus;
			if (n == -1) {
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
		if (lun->lbsize <= 0) {
			respond(r, "no media on this lun");
			return;
		}
		bno = r->ifcall.offset / lun->lbsize;
		nb = (r->ifcall.offset + r->ifcall.count + lun->lbsize - 1) /
			lun->lbsize - bno;
		if (bno + nb > lun->blocks)
			nb = lun->blocks - bno;
		if (bno >= lun->blocks || nb == 0) {
			r->ofcall.count = 0;
			break;
		}
		if (nb * lun->lbsize > MaxIOsize)
			nb = MaxIOsize / lun->lbsize;
		p = malloc(nb * lun->lbsize);
		if (p == nil) {
			respond(r, "no memory");
			return;
		}
		offset = r->ifcall.offset % lun->lbsize;
		len = r->ifcall.count;
		if (offset || (len % lun->lbsize)) {
			lun->offset = r->ifcall.offset / lun->lbsize;
			n = SRread(lun, p, nb * lun->lbsize);
			if (n == -1) {
				free(p);
				respond(r, "IO error");
				return;
			}
			if (offset + len > n)
				len = n - offset;
		}
		memmove(p + offset, r->ifcall.data, len);
		lun->offset = r->ifcall.offset / lun->lbsize;
		n = SRwrite(lun, p, nb * lun->lbsize);
		if (n == -1) {
			free(p);
			respond(r, "IO error");
			return;
		}
		if (offset + len > n)
			len = n - offset;
		r->ofcall.count = len;
		free(p);
		break;
	}
	r->ofcall.count = n;
	respond(r, nil);
}

Srv iscsisrv = {
	.attach	= rattach,
	.walk1	= rwalk1,
	.open	= ropen,
	.read	= rread,
	.write	= rwrite,
	.stat	= rstat,
};

void
usage(void)
{
	fprint(2, "usage: %s [-dD] [-m mtpt] [-s srvname] host target\n", argv0);
	exits("Usage");
}

void
main(int argc, char **argv)
{
	char *srvname, *mntname;

	mntname = "/n/iscsi";
	srvname = nil;

	ARGBEGIN {
	case 'd':
		debug++;
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
	} ARGEND;

	if (argc < 2)
		usage();
	host = argv[0];
	vol = argv[1];
	if (debug)
		mainmem->flags |= POOL_ANTAGONISM | POOL_PARANOIA | POOL_NOREUSE;

	iscsiconnect(&iscsi);
	if (iscsiinit(&iscsi) < 0)
		sysfatal("iscsiinit failed");
	starttime = time(0);
	owner = getuser();

	postmountsrv(&iscsisrv, srvname, mntname, 0);
	exits(0);
}
