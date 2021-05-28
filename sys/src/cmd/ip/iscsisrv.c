/*
 * iscsisrv target - serve target via iscsi
 *
 * invoked by listen(8) via /bin/service/tcp3260,
 * so tcp connection is on fds 0-2.
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <disk.h>			/* for scsi cmds */
#include <pool.h>
#include "iscsi.h"

#define	ROUNDUP(s, sz)	(((s) + ((sz)-1)) & ~((sz)-1))

enum {
	Noreply,
	Reply,

	Blksz	= 512,
	Maxtargs= 256,

	Inqlen = 36,		/* bytes of inquiry data returned */
};

int net;			/* fd of tcp conn. to target */
int targfd = -1;
int rdonly;
int debug;
vlong claimlen;
ulong time0;
char *targfile;
char *advtarg = "the-target";
char *inquiry = "iscsi disk";

static char sendtargall[] = "SendTargets=All";
static char targnm[] = "TargetName=";
static char hdrdig[] =  "HeaderDigest=";
static char datadig[] = "DataDigest=";
static char maxconns[] = "MaxConnections=";
static char initr2t[] = "InitialR2T=";
static char immdata[] = "ImmediateData=";
static char *agreekeys[] = {
	"MaxBurstLength=",
	"FirstBurstLength=",
	"ErrorRecoveryLevel=",
	nil,
};

Iscsistate istate;

void *
emalloc(uint n)
{
	void *v;

	v = mallocz(n, 1);
	if (v == nil)
		sysfatal("out of memory");
	return v;
}

void *
erealloc(void *v, uint n)
{
	v = realloc(v, n);
	if (v == nil)
		sysfatal("out of memory");
	return v;
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

/* set only the seq. nos. common to all response packets */
void
setbhdrseqs(Pkts *pk)
{
	Iscsicmdreq *req;
	Iscsicmdresp *resp;

	req  = (Iscsicmdreq  *)pk->pkt;
	resp = (Iscsicmdresp *)pk->resppkt;

	/*
	 * by returning expcmdseq one past cmdseq, we're saying that we've
	 * executed commands numbered 1—cmdseq.
	 */
	istate.expcmdseq = getbe4(req->cmdseq) + 1;
	/*
	 * give it what it expects for stsseq
	 */
	istate.stsseq = getbe4(req->expstsseq);
	if (debug)
		fprint(2, "\tsts seq %ld exp cmd seq %ld max cmd seq %ld\n",
			istate.stsseq, istate.expcmdseq, istate.expcmdseq + 1);
	putbe4(resp->stsseq, istate.stsseq);
	putbe4(resp->expcmdseq, istate.expcmdseq);
	putbe4(resp->maxcmdseq, istate.expcmdseq + 1);
}

/*
 * append sense data to response pkt.  only permitted for check condition
 * sense data or ireject returned headers; all other data read must
 * be sent via a data-out packet (see appdata).
 */
void
appsensedata(Resppkt *rp, uchar *data, uint len)
{
	int olen;
	Iscsijustbhdr *resp;

	olen = rp->resplen;
	rp->resplen += ROUNDUP(len, 4);

	rp->resppkt = erealloc(rp->resppkt, rp->resplen);
	resp = (Iscsijustbhdr *)rp->resppkt;
	memmove((char *)resp + olen, data, len);
	/* dseglen excludes padding */
	putbe3(resp->dseglen, getbe3(resp->dseglen) + len);
}

void
appsense(Pkts *pk, uchar *sense, ushort len)
{
	uchar dseg[128];

	if (len + 2 > sizeof dseg)
		sysfatal("dseg too small in appsense for %d bytes", len);
	putbe2(dseg, len);		/* iscsi puts sense length first */
	memmove(dseg + 2, sense, len);
	appsensedata(pk, dseg, len + 2);	/* sense data */
}

/*
 * append non-sense data to data-in pkt.
 */
void
appdata(Datainpkt *rp, uchar *data, uint len)
{
	int olen;
	Iscsijustbhdr *dpkt;

	olen = rp->datalen;
	rp->datalen += ROUNDUP(len, 4);

	rp->datapkt = erealloc(rp->datapkt, rp->datalen);
	dpkt = (Iscsijustbhdr *)rp->datapkt;
	memmove((char *)dpkt + olen, data, len);
	/* dseglen excludes padding */
	putbe3(dpkt->dseglen, getbe3(dpkt->dseglen) + len);
}

/* copy basic header from request to response; there's no dseg yet */
void
req2resp(Iscsijustbhdr *resp, Iscsijustbhdr *req)
{
	memmove(resp, req, Bhdrsz);		/* plausible defaults */
	resp->op &= ~(Immed | Oprsrvd);
	resp->op += Topnopin - Iopnopout;	/* req op -> resp op */
	resp->totahslen = 0;
	memset(resp->dseglen, 0, sizeof resp->dseglen);
}

void
newpkt(Iscsijustbhdr *pkt, int op)
{
	memset(pkt, 0, Bhdrsz);
	pkt->op = op;
}

int
ireject(Pkts *pk)
{
	Iscsijustbhdr *resp;

	free(pk->resppkt);
	pk->resppkt = emalloc(sizeof *pk->resppkt);
	pk->resplen = sizeof *pk->resppkt;

	resp = (Iscsijustbhdr *)pk->resppkt;
	newpkt(resp, Topreject);
	resp->opspfc[0] = Finalpkt;
	resp->opspfc[1] = 4;			/* protocol error */
	putbe4(resp->itt, ~0);
	appsensedata(pk, pk->pkt, Bhdrsz);	/* bad pkt hdr as dseg */
	if (debug)
		fprint(2, "** sent reject pkt\n");
	free(pk->datapkt);	/* discard any data to be returned */
	pk->datapkt = nil;
	return Reply;
}

int
keymatch(char *keyval, char **keys)
{
	int keylen;
	char *p;

	p = strchr(keyval, '=');
	if (p == nil)
		sysfatal("key-value pair missing '='");
	keylen = p - keyval;
	for (; *keys != nil; keys++)
		if (strncmp(keyval, *keys, keylen) == 0)
			return 1;
	return 0;
}

void
opneg(Pkts *pk)
{
	int plen, left, len;
	char *p, *resptxt, *txtstart, *eq;
	Iscsiloginresp *resp;
	Iscsiloginreq *req;

	req = (Iscsiloginreq *)pk->pkt;
	resp = (Iscsiloginresp *)pk->resppkt;
	resptxt = txtstart = (char *)resp->dseg;

	resp->lun[7] = 1;		/* non-zero tsih */

	/*
	 * parse keys, generate responses to some (only non-declarative ones).
	 */
	for (p = (char *)req->dseg, len = pk->dseglen; len > 0;
	    p += plen + 1, len -= plen + 1) {
		/* paranoia: last pair might not be NUL-terminated */
		plen = 0;
		for (left = len; left > 0 && p[plen] != '\0'; --left)
			plen++;
		if (keymatch(p, agreekeys)) {
			strcpy(resptxt, p);		/* agree to value */
			resptxt += strlen(resptxt) + 1;
		} else if (strncmp(p, sendtargall, sizeof sendtargall -1) == 0){
			strcpy(resptxt, targnm);
			strcat(resptxt, advtarg);
			resptxt += strlen(resptxt) + 1;
		} else if (strncmp(p, hdrdig, sizeof hdrdig - 1) == 0 ||
		    strncmp(p, datadig, sizeof datadig - 1) == 0) {
			eq = strchr(p, '=');
			assert(eq);
			memmove(resptxt, p, eq + 1 - p);
			resptxt[eq + 1 - p] = '\0';
			strcat(resptxt, "None");
			resptxt += strlen(resptxt) + 1;
		} else if (strncmp(p, maxconns, sizeof maxconns - 1) == 0) {
			strcpy(resptxt, maxconns);
			strcat(resptxt, "1");
			resptxt += strlen(resptxt) + 1;
		} else if (strncmp(p, initr2t, sizeof initr2t - 1) == 0) {
			strcpy(resptxt, initr2t);
			strcat(resptxt, "No");
			resptxt += strlen(resptxt) + 1;
		} else if (strncmp(p, immdata, sizeof immdata - 1) == 0) {
			strcpy(resptxt, immdata);
			strcat(resptxt, "Yes");
			resptxt += strlen(resptxt) + 1;
		}
		assert(resptxt < txtstart + Maxtargs);
	}

	/* append our own demands */
	/* try to prevent initiator generating data-out pkts */
	strcpy(resptxt, "MaxRecvDataSegmentLength=10485760");
	resptxt += strlen(resptxt) + 1;
	assert(resptxt < txtstart + Maxtargs);

	putbe3(resp->dseglen, resptxt - txtstart);
	pk->resplen = ROUNDUP(resptxt - (char *)pk->resppkt, 4);
	if (debug)
		fprint(2, "negotiated operational params for target %s:\n",
			advtarg);
	dumptext("sent", (char *)resp->dseg, getbe3(resp->dseglen));
}

/*
 * phases: 0, security negotiation
 *	1, operational negotiation
 *	2, there is no number 2
 *	3, full-feature
 */
int
ilogin(Pkts *pk)
{
	int trans, cont, csg, nsg, vmax, vmin;
	Iscsiloginresp *resp;
	Iscsiloginreq *req;

	req = (Iscsiloginreq *)pk->pkt;
	trans = req->opspfc[0] >> 7;
	cont = (req->opspfc[0] >> 6) & 1;
	csg = (req->opspfc[0] >> 2) & 3;
	nsg = req->opspfc[0] & 3;
	vmax = req->opspfc[1];
	vmin = req->opspfc[2];
	if (debug) {
		fprint(2, "login req T %d C %d csg %d nsg %d vmax %d vmin %d\n",
			trans, cont, csg, nsg, vmax, vmin);
		fprint(2, "\tcmd seq %ld exp sts seq %ld\n",
			getbe4(req->cmdseq), getbe4(req->expstsseq));
	}

	/* lun is isid[6], tsih[2] */
	if (req->lun[6] || req->lun[7])
		sysfatal("only one connection per session allowed");
	assert(cont == 0);
	dumptext("got", (char *)req->dseg, getbe3(req->dseglen));

	pk->resplen += Maxtargs + 1;
	pk->resppkt = erealloc(pk->resppkt, sizeof *resp + Maxtargs + 1);

	resp = (Iscsiloginresp *)pk->resppkt;
	resp->opspfc[0] &= ~0300;
	resp->opspfc[0] |= 1<<7;			/* T bit, not C bit */
	resp->stsclass = resp->stsdetail = 0;		/* ok */
	memset(resp->_pad2, 0, sizeof resp->_pad2);	/* reserved */

	switch (csg) {
	case 0:
		sysfatal("not willing to negotiate security; sorry");
	case 1:
		opneg(pk);
		break;
	case 3:
		/* full-feature phase */
		resp->lun[7] = 1;		/* non-zero tsih */
		if (debug)
			fprint(2, "logged in, connected to target %s\n",
				advtarg);
		break;
	default:
		sysfatal("bad csg %d", csg);
	}

	if (debug)
		fprint(2, "-> replying to login req in csg %d\n", csg);
	dumpresppkt(pk->resppkt);
	return Reply;
}

int
itext(Pkts *pk)
{
	char *p, *resptxt, *txtstart;
	int trans, cont, len, plen, left;
	Iscsitextresp *resp;
	Iscsitextreq *req;

	req = (Iscsitextreq *)pk->pkt;
	trans = req->opspfc[0] >> 7;
	cont = (req->opspfc[0] >> 6) & 1;
	if (debug) {
		fprint(2, "text req T %d C %d\n", trans, cont);
		fprint(2, "\tcmd seq %ld exp sts seq %ld\n",
			getbe4(req->cmdseq), getbe4(req->expstsseq));
	}
	assert(cont == 0);
	assert(req->totahslen == 0);

	pk->resplen += Maxtargs + 1;
	pk->resppkt = erealloc(pk->resppkt, sizeof *resp + Maxtargs + 1);

	resp = (Iscsitextresp *)pk->resppkt;
	resptxt = txtstart = (char *)resp->dseg;
	resp->opspfc[0] &= ~0300;
	resp->opspfc[0] |= 1<<7;			/* T bit, not C bit */

	dumptext("got", (char *)req->dseg, pk->dseglen);

	for (p = (char *)req->dseg, len = pk->dseglen; len > 0;
	    p += plen + 1, len -= plen + 1) {
		/* paranoia: last pair might not be NUL-terminated */
		plen = 0;
		for (left = len; left > 0 && p[plen] != '\0'; --left)
			plen++;
		if (strncmp(p, sendtargall, sizeof sendtargall - 1) == 0) {
			strcpy(resptxt, targnm);
			strcat(resptxt, advtarg);
			resptxt += strlen(resptxt) + 1;
			assert(resptxt < txtstart + Maxtargs);
		}
	}
	putbe3(resp->dseglen, resptxt - txtstart);

	if (debug)
		fprint(2, "-> replying to text req");
	pk->resplen = ROUNDUP(resptxt - (char *)pk->resppkt, 4);
	if (debug) {
		if (pk->resplen > 0)
			fprint(2, " with %s", txtstart);
		fprint(2, "\n");
	}
	dumptext("sent", (char *)resp->dseg, getbe3(resp->dseglen));
	return Reply;
}

/*
 * linux's iscsi initiator sends nops at about the rate of
 * one per second, which obscures debugging, so print less.
 * itt != ~0 means `send a reply'; otherwise do not send one.
 * set sequence numbers in state from the packet.
 */
int
inop(Pkts *pk)
{
	int trans, cont;
	Iscsinopresp *resp;
	Iscsinopreq *req;

	req = (Iscsinopreq *)pk->pkt;
	trans = req->opspfc[0] >> 7;
	cont = (req->opspfc[0] >> 6) & 1;
//	fprint(2, "nop req T %d C %d\n", trans, cont);
	if (debug)
		fprint(2, "[nop]");
	USED(trans);
	assert(cont == 0);
	assert(req->totahslen == 0);
	if (debug)
		fprint(2, " dseglen %ld lun %#llux cmd seq %ld expcmd seq %ld ",
			getbe4(req->dseglen), getbe8(req->lun),
			getbe4(req->cmdseq), getbe4(req->expcmdseq));

	if (getbe4(req->itt) == ~0ul)
		return Noreply;

	resp = (Iscsinopresp *)pk->resppkt;
	resp->opspfc[0] &= ~0300;
	resp->opspfc[0] |= 1<<7;		/* T bit, not C bit */
	memset(resp->ttt, ~0, sizeof resp->ttt);
	if (debug)
		fprint(2, "[nop reply]");
	return Reply;
}

void
targopen(void)
{
	if (targfd >= 0)
		return;
	targfd = open(targfile, (rdonly? OREAD: ORDWR));
	if (targfd >= 0)
		return;
	if (!rdonly)	/* user didn't say -r, but target could be read-only */
		targfd = open(targfile, OREAD);
	if (targfd >= 0) {
		rdonly = 1;
		return;
	}
	sysfatal("can't open target %s: %r", targfile);
}

vlong
targlen(void)
{
	vlong len;
	Dir *dir;

	targopen();
	dir = dirfstat(targfd);
	if (dir == nil)
		return 0;
	len = dir->length;
	free(dir);
	return len;
}

void
targread(Pkts *pk, vlong blockno, int nblks)
{
	int n;
	uchar *blks;

	if (nblks <= 0)
		return;
	blks = emalloc(nblks*Blksz);
	if (debug)
		fprint(2, "reading %d block(s) @ block %lld of %s\n",
			nblks, blockno, targfile);
	targopen();
	if (seek(targfd, blockno*Blksz, 0) < 0)
		sysfatal("seek on target failed: %r");
	n = read(targfd, blks, nblks*Blksz);
	if (n < 0)
		n = 0;
	appdata(pk, blks, n);
	free(blks);
}

void
chkcond(Pkts *pk)
{
	uchar sense[18];

	pk->resppkt->opspfc[2] = 2;		/* status: check condition */

	memset(sense, 0, sizeof sense);
	sense[0] = 0x70;			/* sense data format */
	sense[7] = sizeof sense - 7;
	sense[12] = 5;				/* illegal request */
	sense[13] = 0x25;			/* lun unsupported */
	appsense(pk, sense, sizeof sense);
}

void
targwrite(Pkts *pk, vlong blockno, int nblks)
{
	int n;
	uchar *blks;
	Iscsicmdreq *req;

	if (nblks <= 0)
		return;
	/* write dseg of dseglen bytes to target */
	req = (Iscsicmdreq *)pk->pkt;
	blks = (uchar *)req + Bhdrsz;
	if (debug)
		fprint(2, "writing %d block(s) @ block %lld of %s\n",
			nblks, blockno, targfile);
	if(nblks*Blksz > getbe3(req->dseglen)) {
		/*
		 * if this happens, the initiator will send data-out pkts
		 * with the remaining data.  we try to prevent this by
		 * setting MaxRecvDataSegmentLength=10485760 during login
		 * negotiation.
		 */
		fprint(2, "** nblks %d * Blksz %d = %d > dseglen %lud\n",
			nblks, Blksz, nblks*Blksz, getbe3(req->dseglen));
		chkcond(pk);
		return;
	}
	targopen();
	if (seek(targfd, blockno*Blksz, 0) < 0)
		sysfatal("seek on target failed: %r");
	n = write(targfd, blks, nblks*Blksz);
	if (n != nblks*Blksz)
		chkcond(pk);			/* write error */
}

int
getcdblun(Pkts *pk)
{
	int lun;
	Iscsicmdreq *req;

	req = (Iscsicmdreq *)pk->pkt;
	lun = req->cdb[1] >> 5;
	if (lun != 0) {
		fprint(2, "unsupported non-zero lun %d\n", lun);
		chkcond(pk);
		return -1;
	}
	return lun;
}

void
cmdinq(Pkts *pk)
{
	int alen, lun, evpd, page;
	uchar inq[Inqlen];
	Iscsicmdreq *req;

	req = (Iscsicmdreq *)pk->pkt;
	if (debug)
		fprint(2, "inquiry\n");
	lun = getcdblun(pk);
	if (lun < 0)
		return;
	evpd = req->cdb[1] & 1;
	if (evpd != 0) {
		fprint(2, "** evpd %d in inquiry\n", evpd);
		chkcond(pk);
		return;
	}
	page = req->cdb[2];
	if (page != 0)
		fprint(2, "** page %d in inquiry\n", page);
	alen = req->cdb[4];
	if (debug)
		fprint(2, "req alloc len %d\n", alen);	/* often 36 */

	/* return a plausible inquiry string for a disk */
	memset(inq, 0, Inqlen);
	inq[0] = 0;			/* disk; tape is 1 */
	inq[2] = 2;			/* scsi-2 device */
	inq[3] = 2;			/* scsi-2 inq data format */
	inq[4] = Inqlen - 4;		/* additional length */
	inq[7] = 1<<6 | 1<<5 | 1<<4;	/* wbus32 | wbus16 | sync */
	memmove((char*)&inq[8],  "plan 9  " "the-target      " "1.00", Inqlen-8);
	appdata(pk, inq, (alen > Inqlen? Inqlen: alen));
}

uchar *
newpage(uchar *p, int page, int len)
{
	*p++ = page;
	*p++ = len;
	return p + len;
}

void
cmdmodesense(Pkts *pk)
{
	int alen, lun, page, rlen;
	uvlong bytes;
	uchar *p;
	uchar sense[255];
	Iscsicmdreq *req;

	req = (Iscsicmdreq *)pk->pkt;
	page = req->cdb[2] & ((1<<6) - 1);
	if (debug)
		fprint(2, "mode sense (6) page %d\n", page);
	lun = getcdblun(pk);
	if (lun < 0)
		return;
	/* req->cdb[4] is bytes permitted for sense data */
	alen = req->cdb[4];
	if (alen > sizeof sense)
		sysfatal("sense array too small (%d bytes for %d asked)",
			sizeof sense, alen);

	memset(sense, 0, sizeof sense);
	/* mode parameter header */
	sense[0] = sizeof sense;
	sense[1] = 0;			/* medium type */
	sense[2] = 0;			/* device-specific param for disk */
	sense[3] = 1 * 8;		/* block descriptor len (for 1 bd) */
	/* block descriptor */
	sense[4] = 0;			/* density */
	bytes = claimlen? claimlen: targlen();
	putbe3(sense + 5, bytes/Blksz);
	putbe3(sense + 9, Blksz);
	p = sense + 4 + 8;

	/*
	 * only pages 63 (all) & 8 are requested by linux.
	 * return pages in page number order.
	 */
	if (page == 63 || page == 2) {
		putbe2(p + 10, 16);	/* max sectors per transfer */
		p = newpage(p, 2, 14);	/* disconnect/reconnect page */
	}
	if (page == 63 || page == 3) {
		putbe2(p + 12, Blksz);
		p = newpage(p, 3, 22);	/* format device page */
	}
	if (page == 63 || page == 8)
		p = newpage(p, 8, 10);	/* caching page */
	if (page == 63 || page == 0xa)
		p = newpage(p, 0xa, 6);	/* control page */

	assert(p <= sense + sizeof sense);
	sense[0] = rlen = p - sense;	/* amount we want to return */

	if (rlen > alen)
		rlen = alen;		/* truncate result */
	appdata(pk, sense, rlen);
}

void
cmdreqsense(Pkts *pk)
{
	int lun, alen;
	uchar sense[18];
	Iscsicmdreq *req;

	req = (Iscsicmdreq *)pk->pkt;
	lun = getcdblun(pk);
	if (lun < 0)
		return;
	alen = req->cdb[4];
	if (alen >= sizeof sense) {
		/* report ok */
		memset(sense, 0, sizeof sense);
		sense[0] = 0x70;		/* sense data format */
		sense[7] = sizeof sense - 7;
		appdata(pk, sense, sizeof sense);
	}
}

//	ScmdRewind	= 0x01,		/* rezero/rewind */
//	ScmdFormat	= 0x04,		/* format unit */
//	ScmdRblimits	= 0x05,		/* read block limits */
//	ScmdSeek	= 0x0B,		/* seek */
//	ScmdFmark	= 0x10,		/* write filemarks */
//	ScmdSpace	= 0x11,		/* space forward/backward */
//	ScmdMselect6	= 0x15,		/* mode select */
//	ScmdMselect10	= 0x55,		/* mode select */
//	ScmdMsense10	= 0x5A,		/* mode sense */
//	ScmdStart	= 0x1B,		/* start/stop unit */
//	ScmdRcapacity16	= 0x9e,		/* long read capacity */
//	ScmdRformatcap	= 0x23,		/* read format capacity */
//	ScmdExtseek	= 0x2B,		/* extended seek */
	// 0xa1 is ata command pass through (12)
	// 0x85 is ata command pass through (16)

int
execcdb(Pkts *pk)
{
	int rd, wr;
	vlong bytes;
	uchar *cdb;
	uchar cap[8];
	Iscsicmdreq *req;

	req = (Iscsicmdreq *)pk->pkt;
	rd = (req->opspfc[0] >> 6) & 1;
	wr = (req->opspfc[0] >> 5) & 1;

	if (debug)
		fprint(2, "=> scsi cmd: ");
	cdb = req->cdb;
	switch (cdb[0]) {
	case ScmdInq:			/* inquiry */
		cmdinq(pk);
		break;
	case ScmdTur:			/* test unit ready */
	case ScmdRsense:		/* request sense (error status) */
		if (debug)
			fprint(2, "%s\n", cdb[0] == ScmdTur? "test unit ready":
				"request sense (6)");
		cmdreqsense(pk);
		break;
	case ScmdRcapacity:		/* read capacity */
		if (debug)
			fprint(2, "read capacity\n");
		bytes = claimlen? claimlen: targlen();
		putbe4(cap, bytes/Blksz - 1);
		putbe4(cap+4, Blksz);
		appdata(pk, cap, sizeof cap);
		break;
	case ScmdMsense6:		/* mode sense */
		cmdmodesense(pk);
		break;
	case ScmdExtread:		/* extended read (10 bytes) */
		if (debug)
			fprint(2, "extread\n");
		if (!rd)
			return ireject(pk);
		targread(pk, getbe4(cdb + 2), cdb[7]<<8 | cdb[8]);
		break;
	case ScmdRead:			/* read (6 bytes) */
		if (debug)
			fprint(2, "read\n");
		targread(pk, getbe4(cdb + 1) & ((1<<29)-1), cdb[4]);
		break;
	case ScmdRead16:
		if (debug)
			fprint(2, "read16\n");
		// adjust cdb offsets:
		// targread(pk, getbe4(cdb + 2), cdb[7]<<8 | cdb[8]);
		return ireject(pk);
	case ScmdExtwrite:		/* extended write (10 bytes) */
	case ScmdExtwritever:		/* extended write and verify (10) */
		if (debug)
			fprint(2, "extwrite\n");
		if (!wr || rdonly)
			return ireject(pk);
		targwrite(pk, getbe4(cdb + 2), cdb[7]<<8 | cdb[8]);
		break;
	case ScmdWrite:			/* write */
	case ScmdWrite16:		/* long write (16 bytes) */
		if (!wr || rdonly)
			return ireject(pk);
		// adjust cdb offsets
		// targwrite(pk, getbe4(cdb + 2), cdb[7]<<8 | cdb[8]);
		return ireject(pk);
	default:
		if (debug)
			fprint(2, "** unknown scsi cmd %#x in cmd req\n", cdb[0]);
		/*
		 * apparently ireject is too big a club, at least for the
		 * the linux initiator.
		 */
		chkcond(pk);
		break;
	}
	return Reply;
}

/*
 * process an incoming scsi command (includes cdb)
 *
 * for some reason, the iscsi spec. allows immediate data for
 * write operations, so the entire exchange is just cmd request
 * and cmd response, yet for read operations, immediate data is
 * not allowed (except for check condition sense data), so the
 * is cmd request, data out, cmd response.
 */
int
icmd(Pkts *pk)
{
	int follow, rd, wr, attr, rlen, repl;
	vlong lun;
	Iscsicmdresp *resp;
	Iscsicmdreq *req;
	Iscsidatain *dpkt;

	req = (Iscsicmdreq *)pk->pkt;
	follow = req->opspfc[0] >> 7;
	rd = (req->opspfc[0] >> 6) & 1;
	wr = (req->opspfc[0] >> 5) & 1;
	attr = req->opspfc[0] & 7;
	if (debug)
		fprint(2, "cmd immed %d req F %d R %d W %d attr %d\n",
			pk->immed, follow, rd, wr, attr);
	assert(req->totahslen == 0);
	if (follow == 0 && wr == 0)
		sysfatal("write cmd req with no following data");

	if (debug) {
		fprint(2, "cdb: ");
		dump(req->cdb, 16);		/* decode cdb */
	}

	if (rd && wr)
		sysfatal("don't support bidirectional transfers");

	resp = (Iscsicmdresp *)pk->resppkt;
	resp->opspfc[0] = 1<<7;		/* final pkt of this response */
	resp->opspfc[1] = 0;		/* response: cmd completed (optimism) */
	resp->opspfc[2] = 0;		/* good status (optimism) */
	memset(resp->snacktag, 0, sizeof resp->snacktag);
	memset(resp->readresid, 0, sizeof resp->readresid);
	memset(resp->resid, 0, sizeof resp->resid);
	memset(resp->expdataseq, 0, sizeof resp->expdataseq);
	setbhdrseqs(pk);
	if (debug)
		fprint(2, "\texp xfer len %ld cmd seq %ld exp sts seq %ld\n",
			getbe4(req->expxferlen), getbe4(req->cmdseq),
			getbe4(req->expstsseq));

	lun = getbe8(req->lun);
	if (lun != 0) {
		fprint(2, "unsupported non-zero lun %,llud (%#llux) in cmd req\n",
			lun, lun);
		chkcond(pk);
		return Reply;
	}
	if (rd) {
		/* clone response packet to get initial data-in packet */
		pk->datalen = sizeof *pk->datapkt;
		pk->datapkt = dpkt = emalloc(pk->datalen);
		memmove(dpkt, resp, pk->datalen);

		dpkt->op = Topdatain;
		putbe4(dpkt->ttt, ~0);
		putbe4(dpkt->buffoff, 0);
	}

	repl = execcdb(pk);

	/* in case of a realloc above */
	resp = (Iscsicmdresp *)pk->resppkt;
	dpkt = pk->datapkt;

	putbe4(resp->expdataseq, 0);
	putbe4(resp->readresid, 0);
	putbe4(resp->resid, 0);

	if (rd) {
		/* if execcdb produced a data-in packet, send it */
		if (pk->datalen > Bhdrsz) {
			rlen = ROUNDUP(pk->datalen, 4);
			if (debug)
				fprint(2, "-> sending data-in pkt of %d bytes\n",
					rlen);
			if (write(net, dpkt, rlen) != rlen)
				sysfatal("error sending data pkt: %r");
		}
		free(dpkt);
		pk->datapkt = nil;
	}
	if (debug && repl) {
		fprint(2, "-> replying to cmd req, %d bytes:\n", pk->resplen);
		dump(resp, Bhdrsz);
		if (pk->resplen > Bhdrsz) {
			fprint(2, "\t");
			dump((uchar *)resp + Bhdrsz, pk->resplen - Bhdrsz);
		}
	}
	return repl;
}

int
itask(Pkts *pk)
{
	Iscsitaskresp *resp;
	Iscsitaskreq *req;

	req = (Iscsitaskreq *)pk->pkt;
	if (debug)
		fprint(2, "task req func %d\n", req->opspfc[0] & 0177);
	assert(req->totahslen == 0);

	resp = (Iscsitaskresp *)pk->resppkt;
	resp->opspfc[0] = 1<<7;
	resp->opspfc[1] = 0;		/* done */

	if (debug)
		fprint(2, "-> replying to task req\n");
	return Reply;
}

int
ilogout(Pkts *pk)
{

	int trans, cont, csg, nsg, vmax, vmin;
	Iscsilogoutresp *resp;
	Iscsilogoutreq *req;

	req = (Iscsilogoutreq *)pk->pkt;
	trans = req->opspfc[0] >> 7;
	cont = (req->opspfc[0] >> 6) & 1;
	csg = (req->opspfc[0] >> 2) & 3;
	nsg = req->opspfc[0] & 3;
	vmax = req->opspfc[1];
	vmin = req->opspfc[2];
	if (debug) {
		fprint(2, "logout req T %d C %d csg %d nsg %d vmax %d vmin %d\n",
			trans, cont, csg, nsg, vmax, vmin);
		fprint(2, "\tcmd seq %ld exp sts seq %ld\n",
			getbe4(req->cmdseq), getbe4(req->expstsseq));
	}

	/* lun is isid[6], tsih[2] */
	if (req->lun[6] || req->lun[7])
		sysfatal("only one connection per session allowed");
	assert(cont == 0);

	pk->resppkt = erealloc(pk->resppkt, sizeof *resp);

	resp = (Iscsilogoutresp *)pk->resppkt;
	resp->opspfc[0] &= ~0300;
	resp->opspfc[0] |= 1<<7;			/* T bit, not C bit */
	memset(resp->_pad2, 0, sizeof resp->_pad2);	/* reserved */

	if (debug)
		fprint(2, "-> replying to logout req in csg %d\n", csg);
	dumpresppkt(pk->resppkt);
	return Reply;
}

void
process(int net, Iscsijustbhdr *bhdr)
{
	int op, repl, rlen;
	long n;
	uchar *pkt;
	Pkts *pk;

	op = bhdr->op & ~(Immed | Oprsrvd);

	pk = emalloc(sizeof *pk);	/* holds pointers to in & out pkts */
	pk->immed = (bhdr->op & Immed) != 0;
	pk->totahslen = bhdr->totahslen * sizeof(ulong);
	pk->dseglen = getbe3(bhdr->dseglen);
	pk->itt = getbe4(bhdr->itt);
	if (op != Iopnopout && debug)
		fprint(2, "\n<- iscsi op %#x totahslen %ld dseglen %ld itt %ld\n",
			op, pk->totahslen, pk->dseglen, pk->itt);

	/*
	 * read the rest of the packet: variable bhdr part, ahses & data
	 * rounded to next word.
	 */
	n = (Bhdrsz - sizeof *bhdr) + pk->totahslen + ROUNDUP(pk->dseglen, 4);
	pkt = emalloc(sizeof *bhdr + n);
	memmove(pkt, bhdr, sizeof *bhdr);

	if (readn(net, pkt + sizeof *bhdr, n) != n)
		sysfatal("truncated packet read");

	pk->pkt = pkt;
	pk->len = n;

	/* allocate response pkt and fill w plausible default values */
	pk->resplen = sizeof *pk->resppkt;
	pk->resppkt = emalloc(pk->resplen);
	req2resp((Iscsijustbhdr *)pk->resppkt, (Iscsijustbhdr *)pk->pkt);
	repl = 0;

	switch (op) {
	case Iopnopout:
		repl = inop(pk);
		break;
	case Iopcmd:
		repl = icmd(pk);
		break;
	case Ioptask:
		repl = itask(pk);
		break;
	case Ioplogin:
		repl = ilogin(pk);
		break;
	case Ioptext:
		repl = itext(pk);
		break;
	case Ioplogout:
		repl = ilogout(pk);
		break;

	case Iopdataout:		/* do not want */
	case Iopsnack:
		repl = ireject(pk);
		break;
	default:
		sysfatal("bad iscsi opcode %#x", op);
	}
	if (repl) {
		rlen = ROUNDUP(pk->resplen, 4);
		if (write(net, pk->resppkt, rlen) != rlen)
			sysfatal("error sending response pkt: %r");
	}
	free(pk->resppkt);
	free(pkt);
	memset(pk, 0, sizeof *pk);
	free(pk);
}

void
usage(void)
{
	fprint(2, "usage: %s [-dr] [-l len] [-a dialstring] target\n", argv0);
	exits("usage");
}

void
callhandler(void)
{
	Iscsijustbhdr justbhdr;

	if (debug)
		mainmem->flags |= POOL_ANTAGONISM | POOL_PARANOIA | POOL_NOREUSE;

	if (debug)
		fprint(2, "\nserving target %s\n", targfile);
	while (readn(net, &justbhdr, sizeof justbhdr) == sizeof justbhdr)
		process(net, &justbhdr);
	if (debug)
		fprint(2, "\ninitiator closed connection\n");
}

void
tcpserver(char *dialstring)
{
	char dir[40], ndir[40];
	int ctl, nctl;

	ctl = announce(dialstring, dir);
	if (ctl < 0) {
		fprint(2, "Can't announce on %s: %r\n", dialstring);
		exits("announce");
	}

	for (;;) {
		nctl = listen(dir, ndir);
		if (nctl < 0) {
			fprint(2, "Listen failed: %r\n");
			exits("listen");
		}

		switch(rfork(RFFDG|RFNOWAIT|RFPROC)) {
		case -1:
			close(nctl);
			fprint(2, "failed to fork, exiting: %r\n");
			exits("fork");
		case 0:
			net = accept(nctl, ndir);
			if (net < 0) {
				fprint(2, "Accept failed: %r\n");
				exits("accept");
			}
			callhandler();
			exits(nil);
		default:
			close(nctl);
		}
	}
}


void
main(int argc, char **argv)
{
	char *dialstring;

	quotefmtinstall();
	time0 = time(0);
	debug = 0;
	dialstring = nil;

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'l':
		claimlen = atoll(EARGF(usage()));
		break;
	case 'r':
		rdonly = 1;
		break;
	case 'a':
		dialstring = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if (argc != 1)
		usage();
	targfile = argv[0];

	if (dialstring) {
		tcpserver(dialstring);
		exits(nil);
	}
	net = 0;
	callhandler();
	exits(nil);
}
