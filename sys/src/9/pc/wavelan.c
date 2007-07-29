/*
	Lucent Wavelan IEEE 802.11 pcmcia.
	There is almost no documentation for the card.
	the driver is done using both the FreeBSD, Linux and
	original Plan 9 drivers as `documentation'.

	Has been used with the card plugged in during all up time.
	no cards removals/insertions yet.

	For known BUGS see the comments below. Besides,
	the driver keeps interrupts disabled for just too
	long. When it gets robust, locks should be revisited.

	BUGS: check endian, alignment and mem/io issues;
	      receive watchdog interrupts.
	TODO: automatic power management;
	      multicast filtering;
	      improve locking.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "etherif.h"
#include "wavelan.h"

enum
{
	MSperTick=	50,	/* ms between ticks of kproc */
};

/*
 * When we're using a PCI device and memory-mapped I/O, 
 * the registers are spaced out as though each takes 32 bits,
 * even though they are only 16-bit registers.  Thus, 
 * ctlr->mmb[reg] is the right way to access register reg,
 * even though a priori you'd expect to use ctlr->mmb[reg/2].
 */
void
csr_outs(Ctlr *ctlr, int reg, ushort arg)
{
	if(ctlr->mmb)
		ctlr->mmb[reg] = arg;
	else
		outs(ctlr->iob+reg, arg);
}

ushort
csr_ins(Ctlr *ctlr, int reg)
{
	if(ctlr->mmb)
		return ctlr->mmb[reg];
	else
		return ins(ctlr->iob+reg);
}

static void
csr_ack(Ctlr *ctlr, int ev)
{
	csr_outs(ctlr, WR_EvAck, ev);
}

static void
csr_inss(Ctlr *ctlr, int reg, void *dat, int ndat)
{
	ushort *rp, *wp;

	if(ctlr->mmb){
		rp = &ctlr->mmb[reg];
		wp = dat;
		while(ndat-- > 0)
			*wp++ = *rp;
	}else
		inss(ctlr->iob+reg, dat, ndat);
}

static void
csr_outss(Ctlr *ctlr, int reg, void *dat, int ndat)
{
	ushort *rp, *wp;

	if(ctlr->mmb){
		rp = dat;
		wp = &ctlr->mmb[reg];
		while(ndat-- > 0)
			*wp = *rp++;
	}else
		outss(ctlr->iob+reg, dat, ndat);
}

// w_... routines do not ilock the Ctlr and should
// be called locked.

void
w_intdis(Ctlr* ctlr)
{
	csr_outs(ctlr, WR_IntEna, 0);
	csr_ack(ctlr, 0xffff);
}

static void
w_intena(Ctlr* ctlr)
{
	csr_outs(ctlr, WR_IntEna, WEvs);
}

int
w_cmd(Ctlr *ctlr, ushort cmd, ushort arg)
{
	int i, rc;

	for(i=0; i<WTmOut; i++)
		if((csr_ins(ctlr, WR_Cmd)&WCmdBusy) == 0)
			break;
	if(i==WTmOut){
		print("#l%d: issuing cmd %.4ux: %.4ux\n", ctlr->ctlrno, cmd, csr_ins(ctlr, WR_Cmd));
		return -1;
	}

	csr_outs(ctlr, WR_Parm0, arg);
	csr_outs(ctlr, WR_Cmd, cmd);

	for(i=0; i<WTmOut; i++)
		if(csr_ins(ctlr, WR_EvSts)&WCmdEv)
			break;
	if(i==WTmOut){
		/*
		 * WCmdIni can take a really long time.
		 */
		enum { IniTmOut = 2000 };
		for(i=0; i<IniTmOut; i++){
			if(csr_ins(ctlr, WR_EvSts)&WCmdEv)
				break;
			microdelay(100);
		}
		if(i < IniTmOut)
			if(0) print("#l%d: long cmd %.4ux %d\n", ctlr->ctlrno, cmd, i);
		if(i == IniTmOut){
			print("#l%d: execing cmd %.4ux: %.4ux\n", ctlr->ctlrno, cmd, csr_ins(ctlr, WR_EvSts));
			return -1;
		}
	}
	rc = csr_ins(ctlr, WR_Sts);
	csr_ack(ctlr, WCmdEv);

	if((rc&WCmdMsk) != (cmd&WCmdMsk)){
		print("#l%d: cmd %.4ux: status %.4ux\n", ctlr->ctlrno, cmd, rc);
		return -1;
	}
	if(rc&WResSts){
		/*
		 * Don't print; this happens on every WCmdAccWr for some reason.
		 */
		if(0) print("#l%d: cmd %.4ux: status %.4ux\n", ctlr->ctlrno, cmd, rc);
		return -1;
	}
	return 0;
}

static int
w_seek(Ctlr* ctlr, ushort id, ushort offset, int chan)
{
	int i, rc;
	static ushort sel[] = { WR_Sel0, WR_Sel1 };
	static ushort off[] = { WR_Off0, WR_Off1 };

	if(chan != 0 && chan != 1)
		panic("wavelan: bad chan\n");
	csr_outs(ctlr, sel[chan], id);
	csr_outs(ctlr, off[chan], offset);
	for (i=0; i<WTmOut; i++){
		rc = csr_ins(ctlr, off[chan]);
		if((rc & (WBusyOff|WErrOff)) == 0)
			return 0;
	}
	return -1;
}

int
w_inltv(Ctlr* ctlr, Wltv* ltv)
{
	int len;
	ushort code;

	if(w_cmd(ctlr, WCmdAccRd, ltv->type)){
		DEBUG("wavelan: access read failed\n");
		return -1;
	}
	if(w_seek(ctlr,ltv->type,0,1)){
		DEBUG("wavelan: seek failed\n");
		return -1;
	}
	len = csr_ins(ctlr, WR_Data1);
	if(len > ltv->len)
		return -1;
	ltv->len = len;
	if((code=csr_ins(ctlr, WR_Data1)) != ltv->type){
		USED(code);
		DEBUG("wavelan: type %x != code %x\n",ltv->type,code);
		return -1;
	}
	if(ltv->len > 0)
		csr_inss(ctlr, WR_Data1, &ltv->val, ltv->len-1);

	return 0;
}

static void
w_outltv(Ctlr* ctlr, Wltv* ltv)
{
	if(w_seek(ctlr,ltv->type, 0, 1))
		return;
	csr_outss(ctlr, WR_Data1, ltv, ltv->len+1);
	w_cmd(ctlr, WCmdAccWr, ltv->type);
}

void
ltv_outs(Ctlr* ctlr, int type, ushort val)
{
	Wltv ltv;

	ltv.len = 2;
	ltv.type = type;
	ltv.val = val;
	w_outltv(ctlr, &ltv);
}

int
ltv_ins(Ctlr* ctlr, int type)
{
	Wltv ltv;

	ltv.len = 2;
	ltv.type = type;
	ltv.val = 0;
	if(w_inltv(ctlr, &ltv))
		return -1;
	return ltv.val;
}

static void
ltv_outstr(Ctlr* ctlr, int type, char* val)
{
	Wltv ltv;
	int len;

	len = strlen(val);
	if(len > sizeof(ltv.s))
		len = sizeof(ltv.s);
	memset(&ltv, 0, sizeof(ltv));
	ltv.len = (sizeof(ltv.type)+sizeof(ltv.slen)+sizeof(ltv.s))/2;
	ltv.type = type;

//	This should be ltv.slen = len; according to Axel Belinfante
	ltv.slen = len;	

	strncpy(ltv.s, val, len);
	w_outltv(ctlr, &ltv);
}

static char Unkname[] = "who knows";
static char Nilname[] = "card does not tell";

static char*
ltv_inname(Ctlr* ctlr, int type)
{
	static Wltv ltv;
	int len;

	memset(&ltv,0,sizeof(ltv));
	ltv.len = WNameLen/2+2;
	ltv.type = type;
	if(w_inltv(ctlr, &ltv))
		return Unkname;
	len = ltv.slen;
	if(len == 0 || ltv.s[0] == 0)
		return Nilname;
	if(len >= sizeof ltv.s)
		len = sizeof ltv.s - 1;
	ltv.s[len] = '\0';
	return ltv.s;
}

static int
w_read(Ctlr* ctlr, int type, int off, void* buf, ulong len)
{
	if(w_seek(ctlr, type, off, 1)){
		DEBUG("wavelan: w_read: seek failed");
		return 0;
	}
	csr_inss(ctlr, WR_Data1, buf, len/2);

	return len;
}

static int
w_write(Ctlr* ctlr, int type, int off, void* buf, ulong len)
{
	if(w_seek(ctlr, type, off, 0)){
		DEBUG("wavelan: w_write: seek failed\n");
		return 0;
	}

	csr_outss(ctlr, WR_Data0, buf, len/2);
	csr_outs(ctlr, WR_Data0, 0xdead);
	csr_outs(ctlr, WR_Data0, 0xbeef);
	if(w_seek(ctlr, type, off + len, 0)){
		DEBUG("wavelan: write seek failed\n");
		return 0;
	}
	if(csr_ins(ctlr, WR_Data0) == 0xdead && csr_ins(ctlr, WR_Data0) == 0xbeef)
		return len;

	DEBUG("wavelan: Hermes bug byte.\n");
	return 0;
}

static int
w_alloc(Ctlr* ctlr, int len)
{
	int rc;
	int i,j;

	if(w_cmd(ctlr, WCmdMalloc, len)==0)
		for (i = 0; i<WTmOut; i++)
			if(csr_ins(ctlr, WR_EvSts) & WAllocEv){
				csr_ack(ctlr, WAllocEv);
				rc=csr_ins(ctlr, WR_Alloc);
				if(w_seek(ctlr, rc, 0, 0))
					return -1;
				len = len/2;
				for (j=0; j<len; j++)
					csr_outs(ctlr, WR_Data0, 0);
				return rc;
			}
	return -1;
}

static int
w_enable(Ether* ether)
{
	Wltv ltv;
	Ctlr* ctlr = (Ctlr*) ether->ctlr;

	if(!ctlr)
		return -1;

	w_intdis(ctlr);
	w_cmd(ctlr, WCmdDis, 0);
	w_intdis(ctlr);
	if(w_cmd(ctlr, WCmdIni, 0))
		return -1;
	w_intdis(ctlr);

	ltv_outs(ctlr, WTyp_Tick, 8);
	ltv_outs(ctlr, WTyp_MaxLen, ctlr->maxlen);
	ltv_outs(ctlr, WTyp_Ptype, ctlr->ptype);
 	ltv_outs(ctlr, WTyp_CreateIBSS, ctlr->createibss);
	ltv_outs(ctlr, WTyp_RtsThres, ctlr->rtsthres);
	ltv_outs(ctlr, WTyp_TxRate, ctlr->txrate);
	ltv_outs(ctlr, WTyp_ApDens, ctlr->apdensity);
	ltv_outs(ctlr, WTyp_PMWait, ctlr->pmwait);
	ltv_outs(ctlr, WTyp_PM, ctlr->pmena);
	if(*ctlr->netname)
		ltv_outstr(ctlr, WTyp_NetName, ctlr->netname);
	if(*ctlr->wantname)
		ltv_outstr(ctlr, WTyp_WantName, ctlr->wantname);
	ltv_outs(ctlr, WTyp_Chan, ctlr->chan);
	if(*ctlr->nodename)
		ltv_outstr(ctlr, WTyp_NodeName, ctlr->nodename);
	ltv.len = 4;
	ltv.type = WTyp_Mac;
	memmove(ltv.addr, ether->ea, Eaddrlen);
	w_outltv(ctlr, &ltv);

	ltv_outs(ctlr, WTyp_Prom, (ether->prom?1:0));

	if(ctlr->hascrypt && ctlr->crypt){
		ltv_outs(ctlr, WTyp_Crypt, ctlr->crypt);
		ltv_outs(ctlr, WTyp_TxKey, ctlr->txkey);
		w_outltv(ctlr, &ctlr->keys);
		ltv_outs(ctlr, WTyp_XClear, ctlr->xclear);
	}

	// BUG: set multicast addresses

	if(w_cmd(ctlr, WCmdEna, 0)){
		DEBUG("wavelan: Enable failed");
		return -1;
	}
	ctlr->txdid = w_alloc(ctlr, 1518 + sizeof(WFrame) + 8);
	ctlr->txmid = w_alloc(ctlr, 1518 + sizeof(WFrame) + 8);
	if(ctlr->txdid == -1 || ctlr->txmid == -1)
		DEBUG("wavelan: alloc failed");
	ctlr->txbusy = 0;
	w_intena(ctlr);
	return 0;
}

static void
w_rxdone(Ether* ether)
{
	Ctlr* ctlr = (Ctlr*) ether->ctlr;
	int len, sp;
	WFrame f;
	Block* bp=0;
	Etherpkt* ep;

	sp = csr_ins(ctlr, WR_RXId);
	len = w_read(ctlr, sp, 0, &f, sizeof(f));
	if(len == 0){
		DEBUG("wavelan: read frame error\n");
		goto rxerror;
	}
	if(f.sts&WF_Err){
		goto rxerror;
	}
	switch(f.sts){
	case WF_1042:
	case WF_Tunnel:
	case WF_WMP:
		len = f.dlen + WSnapHdrLen;
		bp = iallocb(ETHERHDRSIZE + len + 2);
		if(!bp)
			goto rxerror;
		ep = (Etherpkt*) bp->wp;
		memmove(ep->d, f.addr1, Eaddrlen);
		memmove(ep->s, f.addr2, Eaddrlen);
		memmove(ep->type,&f.type,2);
		bp->wp += ETHERHDRSIZE;
		if(w_read(ctlr, sp, WF_802_11_Off, bp->wp, len+2) == 0){
			DEBUG("wavelan: read 802.11 error\n");
			goto rxerror;
		}
		bp->wp = bp->rp+(ETHERHDRSIZE+f.dlen);
		break;
	default:
		len = ETHERHDRSIZE + f.dlen + 2;
		bp = iallocb(len);
		if(!bp)
			goto rxerror;
		if(w_read(ctlr, sp, WF_802_3_Off, bp->wp, len) == 0){
			DEBUG("wavelan: read 800.3 error\n");
			goto rxerror;
		}
		bp->wp += len;
	}

	ctlr->nrx++;
	etheriq(ether,bp,1);
	ctlr->signal = ((ctlr->signal*15)+((f.qinfo>>8) & 0xFF))/16;
	ctlr->noise = ((ctlr->noise*15)+(f.qinfo & 0xFF))/16;
	return;

rxerror:
	freeb(bp);
	ctlr->nrxerr++;
}

static void
w_txstart(Ether* ether)
{
	Etherpkt *pkt;
	Ctlr *ctlr;
	Block *bp;
	int len, off;

	if((ctlr = ether->ctlr) == nil || (ctlr->state & (Attached|Power)) != (Attached|Power) || ctlr->txbusy)
		return;

	if((bp = qget(ether->oq)) == nil)
		return;
	pkt = (Etherpkt*)bp->rp;

	//
	// If the packet header type field is > 1500 it is an IP or
	// ARP datagram, otherwise it is an 802.3 packet. See RFC1042.
	//
	memset(&ctlr->txf, 0, sizeof(ctlr->txf));
	if(((pkt->type[0]<<8)|pkt->type[1]) > 1500){
		ctlr->txf.framectl = WF_Data;
		memmove(ctlr->txf.addr1, pkt->d, Eaddrlen);
		memmove(ctlr->txf.addr2, pkt->s, Eaddrlen);
		memmove(ctlr->txf.dstaddr, pkt->d, Eaddrlen);
		memmove(ctlr->txf.srcaddr, pkt->s, Eaddrlen);
		memmove(&ctlr->txf.type, pkt->type, 2);
		bp->rp += ETHERHDRSIZE;
		len = BLEN(bp);
		off = WF_802_11_Off;
		ctlr->txf.dlen = len+ETHERHDRSIZE-WSnapHdrLen;
		hnputs((uchar*)&ctlr->txf.dat[0], WSnap0);
		hnputs((uchar*)&ctlr->txf.dat[1], WSnap1);
		hnputs((uchar*)&ctlr->txf.len, len+ETHERHDRSIZE-WSnapHdrLen);
	}
	else{
		len = BLEN(bp);
		off = WF_802_3_Off;
		ctlr->txf.dlen = len;
	}
	w_write(ctlr, ctlr->txdid, 0, &ctlr->txf, sizeof(ctlr->txf));
	w_write(ctlr, ctlr->txdid, off, bp->rp, len+2);

	if(w_cmd(ctlr, WCmdReclaim|WCmdTx, ctlr->txdid)){
		DEBUG("wavelan: transmit failed\n");
		ctlr->ntxerr++;
	}
	else{
		ctlr->txbusy = 1;
		ctlr->txtmout = 2;
	}
	freeb(bp);
}

static void
w_txdone(Ctlr* ctlr, int sts)
{
	ctlr->txbusy = 0;
	ctlr->txtmout = 0;
	if(sts & WTxErrEv)
		ctlr->ntxerr++;
	else
		ctlr->ntx++;
}

/* save the stats info in the ctlr struct */
static void
w_stats(Ctlr* ctlr, int len)
{
	int i, rc;
	ulong* p = (ulong*)&ctlr->WStats;
	ulong* pend = (ulong*)&ctlr->end;

	for (i = 0; i < len && p < pend; i++){
		rc = csr_ins(ctlr, WR_Data1);
		if(rc > 0xf000)
			rc = ~rc & 0xffff;
		p[i] += rc;
	}
}

/* send the base station scan info to any readers */
static void
w_scaninfo(Ether* ether, Ctlr *ctlr, int len)
{
	int i, j;
	Netfile **ep, *f, **fp;
	Block *bp;
	WScan *wsp;
	ushort *scanbuf;

	scanbuf = malloc(len*2);
	if(scanbuf == nil)
		return;
	
	for (i = 0; i < len ; i++)
		scanbuf[i] = csr_ins(ctlr, WR_Data1);

	/* calculate number of samples */
	len /= 25;
	if(len == 0)
		goto out;

	i = ether->scan;
	ep = &ether->f[Ntypes];
	for(fp = ether->f; fp < ep && i > 0; fp++){
		f = *fp;
		if(f == nil || f->scan == 0)
			continue;

		bp = iallocb(100*len);
		if(bp == nil)
			break;
		for(j = 0; j < len; j++){
			wsp = (WScan*)(&scanbuf[j*25]);
			if(wsp->ssid_len > 32)
				wsp->ssid_len = 32;
			bp->wp = (uchar*)seprint((char*)bp->wp, (char*)bp->lim,
				"ssid=%.*s;bssid=%E;signal=%d;noise=%d;chan=%d%s\n",
				wsp->ssid_len, wsp->ssid, wsp->bssid, wsp->signal,
				wsp->noise, wsp->chan, (wsp->capinfo&(1<<4))?";wep":"");
		}
		qpass(f->in, bp);
		i--;
	}
out:
	free(scanbuf);
}

static int
w_info(Ether *ether, Ctlr* ctlr)
{
	int sp;
	Wltv ltv;

	sp = csr_ins(ctlr, WR_InfoId);
	ltv.len = ltv.type = 0;
	w_read(ctlr, sp, 0, &ltv, 4);
	ltv.len--;
	switch(ltv.type){
	case WTyp_Stats:
		w_stats(ctlr, ltv.len);
		return 0;
	case WTyp_Scan:
		w_scaninfo(ether, ctlr, ltv.len);
		return 0;
	}
	return -1;
}

/* set scanning interval */
static void
w_scanbs(void *a, uint secs)
{
	Ether *ether = a;
	Ctlr* ctlr = (Ctlr*) ether->ctlr;

	ctlr->scanticks = secs*(1000/MSperTick);
}

static void
w_intr(Ether *ether)
{
	int rc, txid;
	Ctlr* ctlr = (Ctlr*) ether->ctlr;

	if((ctlr->state & Power) == 0)
		return;

	if((ctlr->state & Attached) == 0){
		csr_ack(ctlr, 0xffff);
		csr_outs(ctlr, WR_IntEna, 0);
		return;
	}

	rc = csr_ins(ctlr, WR_EvSts);
	csr_ack(ctlr, ~WEvs);	// Not interested in them
	if(rc & WRXEv){
		w_rxdone(ether);
		csr_ack(ctlr, WRXEv);
	}
	if(rc & WTXEv){
		w_txdone(ctlr, rc);
		csr_ack(ctlr, WTXEv);
	}
	if(rc & WAllocEv){
		ctlr->nalloc++;
		txid = csr_ins(ctlr, WR_Alloc);
		csr_ack(ctlr, WAllocEv);
		if(txid == ctlr->txdid){
			if((rc & WTXEv) == 0)
				w_txdone(ctlr, rc);
		}
	}
	if(rc & WInfoEv){
		ctlr->ninfo++;
		w_info(ether, ctlr);
		csr_ack(ctlr, WInfoEv);
	}
	if(rc & WTxErrEv){
		w_txdone(ctlr, rc);
		csr_ack(ctlr, WTxErrEv);
	}
	if(rc & WIDropEv){
		ctlr->nidrop++;
		csr_ack(ctlr, WIDropEv);
	}
	w_txstart(ether);
}

// Watcher to ensure that the card still works properly and
// to request WStats updates once a minute.
// BUG: it runs much more often, see the comment below.

static void
w_timer(void* arg)
{
	Ether* ether = (Ether*) arg;
	Ctlr* ctlr = (Ctlr*)ether->ctlr;

	ctlr->timerproc = up;
	for(;;){
		tsleep(&up->sleep, return0, 0, MSperTick);
		ctlr = (Ctlr*)ether->ctlr;
		if(ctlr == 0)
			break;
		if((ctlr->state & (Attached|Power)) != (Attached|Power))
			continue;
		ctlr->ticks++;

		ilock(ctlr);

		// Seems that the card gets frames BUT does
		// not send the interrupt; this is a problem because
		// I suspect it runs out of receive buffers and
		// stops receiving until a transmit watchdog
		// reenables the card.
		// The problem is serious because it leads to
		// poor rtts.
		// This can be seen clearly by commenting out
		// the next if and doing a ping: it will stop
		// receiving (although the icmp replies are being
		// issued from the remote) after a few seconds.
		// Of course this `bug' could be because I'm reading
		// the card frames in the wrong way; due to the
		// lack of documentation I cannot know.

		if(csr_ins(ctlr, WR_EvSts)&WEvs){
			ctlr->tickintr++;
			w_intr(ether);
		}

		if((ctlr->ticks % 10) == 0) {
			if(ctlr->txtmout && --ctlr->txtmout == 0){
				ctlr->nwatchdogs++;
				w_txdone(ctlr, WTxErrEv);
				if(w_enable(ether)){
					DEBUG("wavelan: wdog enable failed\n");
				}
				w_txstart(ether);
			}
			if((ctlr->ticks % 120) == 0)
			if(ctlr->txbusy == 0)
				w_cmd(ctlr, WCmdEnquire, WTyp_Stats);
			if(ctlr->scanticks > 0)
			if((ctlr->ticks % ctlr->scanticks) == 0)
			if(ctlr->txbusy == 0)
				w_cmd(ctlr, WCmdEnquire, WTyp_Scan);
		}
		iunlock(ctlr);
	}
	pexit("terminated", 0);
}

void
w_multicast(void *ether, uchar*, int add)
{
	/* BUG: use controller's multicast filter */
	if (add)
		w_promiscuous(ether, 1);
}

void
w_attach(Ether* ether)
{
	Ctlr* ctlr;
	char name[64];
	int rc;

	if(ether->ctlr == 0)
		return;

	snprint(name, sizeof(name), "#l%dtimer", ether->ctlrno);
	ctlr = (Ctlr*) ether->ctlr;
	if((ctlr->state & Attached) == 0){
		ilock(ctlr);
		rc = w_enable(ether);
		iunlock(ctlr);
		if(rc == 0){
			ctlr->state |= Attached;
			kproc(name, w_timer, ether);
		} else
			print("#l%d: enable failed\n",ether->ctlrno);
	}
}

void
w_detach(Ether* ether)
{
	Ctlr* ctlr;
	char name[64];

	if(ether->ctlr == nil)
		return;

	snprint(name, sizeof(name), "#l%dtimer", ether->ctlrno);
	ctlr = (Ctlr*) ether->ctlr;
	if(ctlr->state & Attached){
		ilock(ctlr);
		w_intdis(ctlr);
		if(ctlr->timerproc){
			if(!postnote(ctlr->timerproc, 1, "kill", NExit))
				print("timerproc note not posted\n");
			print("w_detach, killing 0x%p\n", ctlr->timerproc);
		}
		ctlr->state &= ~Attached;
		iunlock(ctlr);
	}
	ether->ctlr = nil;
}

void
w_power(Ether* ether, int on)
{
	Ctlr *ctlr;

	ctlr = (Ctlr*) ether->ctlr;
	ilock(ctlr);
iprint("w_power %d\n", on);
	if(on){
		if((ctlr->state & Power) == 0){
			if (wavelanreset(ether, ctlr) < 0){
				iprint("w_power: reset failed\n");
				iunlock(ctlr);
				w_detach(ether);
				free(ctlr);
				return;
			}
			if(ctlr->state & Attached)
				w_enable(ether);
			ctlr->state |= Power;
		}
	}else{
		if(ctlr->state & Power){
			if(ctlr->state & Attached)
				w_intdis(ctlr);
			ctlr->state &= ~Power;
		}
	}
	iunlock(ctlr);
}

#define PRINTSTAT(fmt,val)	l += snprint(p+l, READSTR-l, (fmt), (val))
#define PRINTSTR(fmt)		l += snprint(p+l, READSTR-l, (fmt))

long
w_ifstat(Ether* ether, void* a, long n, ulong offset)
{
	Ctlr *ctlr = (Ctlr*) ether->ctlr;
	char *k, *p;
	int i, l, txid;

	ether->oerrs = ctlr->ntxerr;
	ether->crcs = ctlr->nrxfcserr;
	ether->frames = 0;
	ether->buffs = ctlr->nrxdropnobuf;
	ether->overflows = 0;

	//
	// Offset must be zero or there's a possibility the
	// new data won't match the previous read.
	//
	if(n == 0 || offset != 0)
		return 0;

	p = malloc(READSTR);
	l = 0;

	PRINTSTAT("Signal: %d\n", ctlr->signal-149);
	PRINTSTAT("Noise: %d\n", ctlr->noise-149);
	PRINTSTAT("SNR: %ud\n", ctlr->signal-ctlr->noise);
	PRINTSTAT("Interrupts: %lud\n", ctlr->nints);
	PRINTSTAT("Double Interrupts: %lud\n", ctlr->ndoubleint);
	PRINTSTAT("TxPackets: %lud\n", ctlr->ntx);
	PRINTSTAT("RxPackets: %lud\n", ctlr->nrx);
	PRINTSTAT("TxErrors: %lud\n", ctlr->ntxerr);
	PRINTSTAT("RxErrors: %lud\n", ctlr->nrxerr);
	PRINTSTAT("TxRequests: %lud\n", ctlr->ntxrq);
	PRINTSTAT("AllocEvs: %lud\n", ctlr->nalloc);
	PRINTSTAT("InfoEvs: %lud\n", ctlr->ninfo);
	PRINTSTAT("InfoDrop: %lud\n", ctlr->nidrop);
	PRINTSTAT("WatchDogs: %lud\n", ctlr->nwatchdogs);
	PRINTSTAT("Ticks: %ud\n", ctlr->ticks);
	PRINTSTAT("TickIntr: %ud\n", ctlr->tickintr);
	k = ((ctlr->state & Attached) ? "attached" : "not attached");
	PRINTSTAT("Card %s", k);
	k = ((ctlr->state & Power) ? "on" : "off");
	PRINTSTAT(", power %s", k);
	k = ((ctlr->txbusy)? ", txbusy" : "");
	PRINTSTAT("%s\n", k);

	if(ctlr->hascrypt){
		PRINTSTR("Keys: ");
		for (i = 0; i < WNKeys; i++){
			if(ctlr->keys.keys[i].len == 0)
				PRINTSTR("none ");
			else if(SEEKEYS == 0)
				PRINTSTR("set ");
			else
				PRINTSTAT("%s ", ctlr->keys.keys[i].dat);
		}
		PRINTSTR("\n");
	}

	// real card stats
	ilock(ctlr);
	PRINTSTR("\nCard stats: \n");
	PRINTSTAT("Status: %ux\n", csr_ins(ctlr, WR_Sts));
	PRINTSTAT("Event status: %ux\n", csr_ins(ctlr, WR_EvSts));
	i = ltv_ins(ctlr, WTyp_Ptype);
	PRINTSTAT("Port type: %d\n", i);
	PRINTSTAT("Transmit rate: %d\n", ltv_ins(ctlr, WTyp_TxRate));
	PRINTSTAT("Current Transmit rate: %d\n",
		ltv_ins(ctlr, WTyp_CurTxRate));
	PRINTSTAT("Channel: %d\n", ltv_ins(ctlr, WTyp_Chan));
	PRINTSTAT("AP density: %d\n", ltv_ins(ctlr, WTyp_ApDens));
	PRINTSTAT("Promiscuous mode: %d\n", ltv_ins(ctlr, WTyp_Prom));
	if(i == WPTypeAdHoc)
		PRINTSTAT("SSID name: %s\n", ltv_inname(ctlr, WTyp_NetName));
	else {
		Wltv ltv;
		PRINTSTAT("Current name: %s\n", ltv_inname(ctlr, WTyp_CurName));
		ltv.type = WTyp_BaseID;
		ltv.len = 4;
		if(w_inltv(ctlr, &ltv))
			print("#l%d: unable to read base station mac addr\n", ether->ctlrno);
		l += snprint(p+l, READSTR-l, "Base station: %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n",
			ltv.addr[0], ltv.addr[1], ltv.addr[2], ltv.addr[3], ltv.addr[4], ltv.addr[5]);
	}
	PRINTSTAT("Net name: %s\n", ltv_inname(ctlr, WTyp_WantName));
	PRINTSTAT("Node name: %s\n", ltv_inname(ctlr, WTyp_NodeName));
	if(ltv_ins(ctlr, WTyp_HasCrypt) == 0)
		PRINTSTR("WEP: not supported\n");
	else {
		if(ltv_ins(ctlr, WTyp_Crypt) == 0)
			PRINTSTR("WEP: disabled\n");
		else{
			PRINTSTR("WEP: enabled\n");
			k = ((ctlr->xclear)? "excluded": "included");
			PRINTSTAT("Clear packets: %s\n", k);
			txid = ltv_ins(ctlr, WTyp_TxKey);
			PRINTSTAT("Transmit key id: %d\n", txid);
		}
	}
	iunlock(ctlr);

	PRINTSTAT("ntxuframes: %lud\n", ctlr->ntxuframes);
	PRINTSTAT("ntxmframes: %lud\n", ctlr->ntxmframes);
	PRINTSTAT("ntxfrags: %lud\n", ctlr->ntxfrags);
	PRINTSTAT("ntxubytes: %lud\n", ctlr->ntxubytes);
	PRINTSTAT("ntxmbytes: %lud\n", ctlr->ntxmbytes);
	PRINTSTAT("ntxdeferred: %lud\n", ctlr->ntxdeferred);
	PRINTSTAT("ntxsretries: %lud\n", ctlr->ntxsretries);
	PRINTSTAT("ntxmultiretries: %lud\n", ctlr->ntxmultiretries);
	PRINTSTAT("ntxretrylimit: %lud\n", ctlr->ntxretrylimit);
	PRINTSTAT("ntxdiscards: %lud\n", ctlr->ntxdiscards);
	PRINTSTAT("nrxuframes: %lud\n", ctlr->nrxuframes);
	PRINTSTAT("nrxmframes: %lud\n", ctlr->nrxmframes);
	PRINTSTAT("nrxfrags: %lud\n", ctlr->nrxfrags);
	PRINTSTAT("nrxubytes: %lud\n", ctlr->nrxubytes);
	PRINTSTAT("nrxmbytes: %lud\n", ctlr->nrxmbytes);
	PRINTSTAT("nrxfcserr: %lud\n", ctlr->nrxfcserr);
	PRINTSTAT("nrxdropnobuf: %lud\n", ctlr->nrxdropnobuf);
	PRINTSTAT("nrxdropnosa: %lud\n", ctlr->nrxdropnosa);
	PRINTSTAT("nrxcantdecrypt: %lud\n", ctlr->nrxcantdecrypt);
	PRINTSTAT("nrxmsgfrag: %lud\n", ctlr->nrxmsgfrag);
	PRINTSTAT("nrxmsgbadfrag: %lud\n", ctlr->nrxmsgbadfrag);
	USED(l);
	n = readstr(offset, a, n, p);
	free(p);
	return n;
}
#undef PRINTSTR
#undef PRINTSTAT

static int
parsekey(WKey* key, char* a) 
{
	int i, k, len, n;
	char buf[WMaxKeyLen];

	len = strlen(a);
	if(len == WMinKeyLen || len == WMaxKeyLen){
		memset(key->dat, 0, sizeof(key->dat));
		memmove(key->dat, a, len);
		key->len = len;

		return 0;
	}
	else if(len == WMinKeyLen*2 || len == WMaxKeyLen*2){
		k = 0;
		for(i = 0; i < len; i++){
			if(*a >= '0' && *a <= '9')
				n = *a++ - '0';
			else if(*a >= 'a' && *a <= 'f')
				n = *a++ - 'a' + 10;
			else if(*a >= 'A' && *a <= 'F')
				n = *a++ - 'A' + 10;
			else
				return -1;
	
			if(i & 1){
				buf[k] |= n;
				k++;
			}
			else
				buf[k] = n<<4;
		}

		memset(key->dat, 0, sizeof(key->dat));
		memmove(key->dat, buf, k);
		key->len = k;

		return 0;
	}

	return -1;
}

int
w_option(Ctlr* ctlr, char* buf, long n)
{
	char *p;
	int i, r;
	Cmdbuf *cb;

	r = 0;

	cb = parsecmd(buf, n);
	if(cb->nf < 2)
		r = -1;
	else if(cistrcmp(cb->f[0], "essid") == 0){
		if(cistrcmp(cb->f[1],"default") == 0)
			p = "";
		else
			p = cb->f[1];
		if(ctlr->ptype == WPTypeAdHoc){
			memset(ctlr->netname, 0, sizeof(ctlr->netname));
			strncpy(ctlr->netname, p, WNameLen);
		}
		else{
			memset(ctlr->wantname, 0, sizeof(ctlr->wantname));
			strncpy(ctlr->wantname, p, WNameLen);
		}
	}
	else if(cistrcmp(cb->f[0], "station") == 0){
		memset(ctlr->nodename, 0, sizeof(ctlr->nodename));
		strncpy(ctlr->nodename, cb->f[1], WNameLen);
	}
	else if(cistrcmp(cb->f[0], "channel") == 0){
		if((i = atoi(cb->f[1])) >= 1 && i <= 16)
			ctlr->chan = i;
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "mode") == 0){
		if(cistrcmp(cb->f[1], "managed") == 0)
			ctlr->ptype = WPTypeManaged;
		else if(cistrcmp(cb->f[1], "wds") == 0)
			ctlr->ptype = WPTypeWDS;
		else if(cistrcmp(cb->f[1], "adhoc") == 0)
			ctlr->ptype = WPTypeAdHoc;
		else if((i = atoi(cb->f[1])) >= 0 && i <= 3)
			ctlr->ptype = i;
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "ibss") == 0){
		if(cistrcmp(cb->f[1], "on") == 0)
			ctlr->createibss = 1;
		else
			ctlr->createibss = 0;
	}
	else if(cistrcmp(cb->f[0], "crypt") == 0){
		if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->crypt = 0;
		else if(cistrcmp(cb->f[1], "on") == 0 && ctlr->hascrypt)
			ctlr->crypt = 1;
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "clear") == 0){
		if(cistrcmp(cb->f[1], "on") == 0)
			ctlr->xclear = 0;
		else if(cistrcmp(cb->f[1], "off") == 0 && ctlr->hascrypt)
			ctlr->xclear = 1;
		else
			r = -1;
	}
	else if(cistrncmp(cb->f[0], "key", 3) == 0){
		if((i = atoi(cb->f[0]+3)) >= 1 && i <= WNKeys){
			ctlr->txkey = i-1;
			if(parsekey(&ctlr->keys.keys[ctlr->txkey], cb->f[1]))
				r = -1;
		}
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "txkey") == 0){
		if((i = atoi(cb->f[1])) >= 1 && i <= WNKeys)
			ctlr->txkey = i-1;
		else
			r = -1;
	}
	else if(cistrcmp(cb->f[0], "pm") == 0){
		if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->pmena = 0;
		else if(cistrcmp(cb->f[1], "on") == 0){
			ctlr->pmena = 1;
			if(cb->nf == 3){
				i = atoi(cb->f[2]);
				// check range here? what are the units?
				ctlr->pmwait = i;
			}
		}
		else
			r = -1;
	}
	else
		r = -2;
	free(cb);

	return r;
}

long
w_ctl(Ether* ether, void* buf, long n)
{
	Ctlr *ctlr;

	if((ctlr = ether->ctlr) == nil)
		error(Enonexist);
	if((ctlr->state & Attached) == 0)
		error(Eshutdown);

	ilock(ctlr);
	if(w_option(ctlr, buf, n)){
		iunlock(ctlr);
		error(Ebadctl);
	}
	if(ctlr->txbusy)
		w_txdone(ctlr, WTxErrEv);
	w_enable(ether);
	w_txstart(ether);
	iunlock(ctlr);

	return n;
}

void
w_transmit(Ether* ether)
{
	Ctlr* ctlr = ether->ctlr;

	if(ctlr == 0)
		return;

	ilock(ctlr);
	ctlr->ntxrq++;
	w_txstart(ether);
	iunlock(ctlr);
}

void
w_promiscuous(void* arg, int on)
{
	Ether* ether = (Ether*)arg;
	Ctlr* ctlr = ether->ctlr;

	if(ctlr == nil)
		error("card not found");
	if((ctlr->state & Attached) == 0)
		error("card not attached");
	ilock(ctlr);
	ltv_outs(ctlr, WTyp_Prom, (on?1:0));
	iunlock(ctlr);
}

void
w_interrupt(Ureg* ,void* arg)
{
	Ether* ether = (Ether*) arg;
	Ctlr* ctlr = (Ctlr*) ether->ctlr;

	if(ctlr == 0)
		return;
	ilock(ctlr);
	ctlr->nints++;
	w_intr(ether);
	iunlock(ctlr);
}

int
wavelanreset(Ether* ether, Ctlr *ctlr)
{
	Wltv ltv;

	iprint("wavelanreset, iob 0x%ux\n", ctlr->iob);
	w_intdis(ctlr);
	if(w_cmd(ctlr,WCmdIni,0)){
		iprint("#l%d: init failed\n", ether->ctlrno);
		return -1;
	}
	w_intdis(ctlr);
	ltv_outs(ctlr, WTyp_Tick, 8);

	ctlr->chan = 0;
	ctlr->ptype = WDfltPType;
	ctlr->txkey = 0;
	ctlr->createibss = 0;
	ctlr->keys.len = sizeof(WKey)*WNKeys/2 + 1;
	ctlr->keys.type = WTyp_Keys;
	if(ctlr->hascrypt = ltv_ins(ctlr, WTyp_HasCrypt))
		ctlr->crypt = 1;
	*ctlr->netname = *ctlr->wantname = 0;
	strcpy(ctlr->nodename, "Plan 9 STA");

	ctlr->netname[WNameLen-1] = 0;
	ctlr->wantname[WNameLen-1] = 0;
	ctlr->nodename[WNameLen-1] =0;

	ltv.type = WTyp_Mac;
	ltv.len	= 4;
	if(w_inltv(ctlr, &ltv)){
		iprint("#l%d: unable to read mac addr\n",
			ether->ctlrno);
		return -1;
	}
	memmove(ether->ea, ltv.addr, Eaddrlen);

	if(ctlr->chan == 0)
		ctlr->chan = ltv_ins(ctlr, WTyp_Chan);
	ctlr->apdensity = WDfltApDens;
	ctlr->rtsthres = WDfltRtsThres;
	ctlr->txrate = WDfltTxRate;
	ctlr->maxlen = WMaxLen;
	ctlr->pmena = 0;
	ctlr->pmwait = 100;
	ctlr->signal = 1;
	ctlr->noise = 1;
	ctlr->state |= Power;

	// free old Ctlr struct if resetting after suspend
	if(ether->ctlr && ether->ctlr != ctlr)
		free(ether->ctlr);

	// link to ether
	ether->ctlr = ctlr;
	ether->mbps = 10;
	ether->attach = w_attach;
	ether->detach = w_detach;
	ether->interrupt = w_interrupt;
	ether->transmit = w_transmit;
	ether->ifstat = w_ifstat;
	ether->ctl = w_ctl;
	ether->power = w_power;
	ether->promiscuous = w_promiscuous;
	ether->multicast = w_multicast;
	ether->scanbs = w_scanbs;
	ether->arg = ether;

	DEBUG("#l%d: irq %d port %lx type %s",
		ether->ctlrno, ether->irq, ether->port,	ether->type);
	DEBUG(" %2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux\n",
		ether->ea[0], ether->ea[1], ether->ea[2],
		ether->ea[3], ether->ea[4], ether->ea[5]);

	return 0;
}

char* wavenames[] = {
	"WaveLAN/IEEE",
	"TrueMobile 1150",
	"Instant Wireless ; Network PC CARD",
	"Instant Wireless Network PC Card",
	"Avaya Wireless PC Card",
	"AirLancer MC-11",
	"INTERSIL;HFA384x/IEEE;Version 01.02;",
	nil,
};
