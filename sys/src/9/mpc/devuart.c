#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/netif.h"

enum {
	FADS823 = 1,

	Rbufsize=	32,	// read buffer size
	Nuart=	3,	/* max per machine (SMC1, SMC2, SCC2) */
	CTLS=	's'&037,
	CTLQ=	'q'&037,
};

enum {
	/* status bits in SCC receive buffer descriptors */
	RxBRK=	1<<7,	/* break ended frame (async hdlc) */
	RxDE=	1<<7,	/* DPLL error (hdlc) */
	RxBOF=	1<<6,	/* BOF ended frame (async hdlc) */
	RxLG=	1<<5,	/* frame too large (hdlc) */
	RxNO=	1<<4,	/* bad bit alignment (hdlc) */
	RxBR=	1<<5,	/* break received during frame (uart) */
	RxFR=	1<<4,	/* framing error (uart) */
	RxPR=	1<<3,	/* parity error (uart) */
	RxAB=	1<<3,	/* frame aborted (hdlc, async hdlc) */
	RxCR=	1<<2,	/* bad CRC (hdlc, async hdlc) */
	RxOV=	1<<1,	/* receiver overrun (all) */
	RxCD=	1<<0,	/* CD lost (all) */

	/* hdlc-specific Rx/Tx BDs */
	TxTC=	1<<10,
};

/*
 *  SMC in UART mode
 */
enum {
	USESMC2 = 0
};

typedef struct Uartsmc Uartsmc;
struct Uartsmc {
	IOCparam;
	ushort	maxidl;
	ushort	idlc;
	ushort	brkln;
	ushort	brkec;
	ushort	brkcr;
	ushort	rmask;
};

/*
 * SCC2 UART parameters
 */
enum {
	USESCC2 = 0,

	/* special mode bits */
	SccAHDLC = 1<<0,
	SccHDLC = 1<<1,
	SccIR = 1<<2,
	SccPPP = 1<<3,
};

typedef struct Uartscc Uartscc;
struct Uartscc {
	SCCparam;
	uchar	rsvd[8];
	ushort	max_idl;
	ushort	idlc;
	ushort	brkcr;
	ushort	parec;
	ushort	frmec;
	ushort	nosec;
	ushort	brkec;
	ushort	brkln;
	ushort	uaddr1;
	ushort	uaddr2;
	ushort	rtemp;
	ushort	toseq;
	ushort	character[8];
	ushort	rccm;
	ushort	rccrp;
	ushort	rlbc;
};

typedef struct UartAHDLC UartAHDLC;
struct UartAHDLC {
	SCCparam;
	ulong	rsvd1;
	ulong	c_mask;
	ulong	c_pres;
	ushort	bof;
	ushort	eof;
	ushort	esc;
	ushort	rsvd2[2];
	ushort	zero;
	ushort	rsvd3;
	ushort	rfthr;
	ushort	resvd4[2];
	ulong	txctl_tbl;
	ulong	rxctl_tbl;
	ushort	nof;
	ushort	rsvd5;
};

typedef struct UartHDLC UartHDLC;
struct UartHDLC {
	SCCparam;
	ulong	rsvd1;
	ulong	c_mask;
	ulong	c_pres;
	ushort	disfc;
	ushort	crcec;
	ushort	abtsc;
	ushort	nmarc;
	ushort	retrc;
	ushort	mflr;
	ushort	max_cnt;
	ushort	rfthr;
	ushort	rfcnt;
	ushort	hmask;
	ushort	haddr[4];
	ushort	tmp;
	ushort	tmp_mb;
};

enum {
	/* SCC events of possible interest here eventually */
	AB=	1<<9,	/* autobaud detected */
	GRA= 1<<7,	/* graceful stop completed */
	CCR= 1<<3,	/* control character detected */

	/* SCC, SMC common interrupt events */
	BSY=	1<<2,	/* receive buffer was busy (overrun) */
	TXB= 1<<1,	/* block sent */
	RXB= 1<<0,	/* block received */

	/* SCC events */
	TXE = 1<<4,	/* transmission error */
	RXF = 1<<3,	/* final block received */

	/* gsmr_l */
	ENR = 2<<4,	/* enable receiver */
	ENT = 1<<4,	/* enable transmitter */
};

typedef struct Uart Uart;
struct Uart
{
	QLock;

	Uart	*elist;		/* next enabled interface */
	char	name[NAMELEN];

	int	x;		/* index: x in SMCx or SCCx */
	int	cpmid;		/* eg, SCC1ID, SMC1ID */
	int	opens;
	uchar	bpc;	/* bits/char */
	uchar	parity;
	uchar	stopb;
	uchar	setup;
	uchar	enabled;
	int	dev;

	ulong	frame;		/* framing errors */
	ulong	perror;
	ulong	overrun;	/* rcvr overruns */
	ulong	crcerr;
	ulong	interrupts;
	int	baud;		/* baud rate */

	/* flow control */
	int	xonoff;		/* software flow control on */
	int	blocked;
	int	modem;		/* hardware flow control on */
	int	cts;		/* ... cts state */
	int	rts;		/* ... rts state */
	Rendez	r;

	/* buffers */
	int	(*putc)(Queue*, int);
	Queue	*iq;
	Queue	*oq;

	int	rdrx;	/* last buffer read */

	Lock	plock;		/* for output variables */
	Block*	outb;	/* currently transmitting Block */

	BD*	rxb;
	BD*	txb;

	SMC*	smc;
	SCC*	scc;
	IOCparam*	param;
	ushort*	brkcr;	/* brkcr location in appropriate block */
	int	brgc;
	int	mode;
	Block*	partial;
	int	loopback;
};

static Uart *uart[Nuart];
static int nuart;

struct Uartalloc {
	Lock;
	Uart *elist;	/* list of enabled interfaces */
} uartalloc;

static void uartintr(Uart*, int);
static void smcuintr(Ureg*, void*);
static void sccuintr(Ureg*, void*);

static	int	sccirq[] = {-1, 0x1E, 0x1D, 0x1C, 0x1B};
static	int	sccid[] = {-1, SCC1ID, SCC2ID, SCC3ID, SCC4ID};
static	int	sccbase[] = {-1, 0xA00, 0xA20, 0xA40, 0xA60};
static	int	sccparam[] = {-1, SCC1P, SCC2P, SCC3P, SCC4P};

static void
uartsetbuf(Uart *up)
{
	IOCparam *p;
	BD *bd;
	int n;
	uchar *buf;

	p = up->param;
	p->rfcr = 0x18;
	p->tfcr = 0x18;
	p->mrblr = Rbufsize;

	// allocate buffer for receive
	// the buffers need to be CACHELINESZ aligned and
	// a multiple of CACHELINESZ long in order for
	// the manul cache coherence to work
	n = 2*ROUND(Rbufsize, CACHELINESZ)+CACHELINESZ-1;
	buf = malloc(n);
	buf = (uchar*)ROUND((long)buf, CACHELINESZ);
	
	if((bd = up->rxb) == nil){
		bd = bdalloc(2);
		up->rxb = bd;
	}
	p->rbase = (ushort)bd;
	bd->status = BDEmpty|BDInt;
	bd->length = 0;
	bd->addr = PADDR(buf);
	bd++;
	bd->status = BDEmpty|BDInt|BDWrap;
	bd->length = 0;
	bd->addr = PADDR(buf + ROUND(Rbufsize, CACHELINESZ));
	dcflush(buf, 2*ROUND(Rbufsize, CACHELINESZ));
	up->rdrx = 0;

	if((bd = up->txb) == nil){
		bd = bdalloc(1);
		up->txb = bd;
	}
	p->tbase = (ushort)bd;
	bd->status = BDWrap|BDInt;
	bd->length = 0;
	bd->addr = 0;
}

static void
smcsetup(Uart *up)
{
	IMM *io;
	Uartsmc *p;
	SMC *smc;

	up->brgc = brgalloc();
	if(up->brgc < 0)
		error(Eio);

	io = ioplock();
	/* step 1 */
	io->papar |= SIBIT(8)|SIBIT(9);
	io->padir &= ~(SIBIT(8)|SIBIT(9));
	io->paodr &= ~(SIBIT(8)|SIBIT(9));

	io->brgc[up->brgc] = baudgen(up->baud, 16) | BaudEnable;

	/* step 3: route clock to SMC2 */
	io->simode = (io->simode & ~0xF0000000) | (up->brgc<<28);

	iopunlock();

	up->param = KADDR(SMC2P);
	uartsetbuf(up);

	cpmop(InitRxTx, SMC2ID, 0);

	/* SMC protocol parameters */
	p = (Uartsmc*)up->param;
	up->brkcr = &p->brkcr;
	p->maxidl = 10;
	p->brkln = 0;
	p->brkec = 0;
	p->brkcr = 1;
	smc = IOREGS(0xA90, SMC);
	smc->smce = 0xff;	/* clear events */
	smc->smcm = BSY|RXB|TXB;	/* enable all possible interrupts */
	up->smc = smc;
	smc->smcmr = ((1+8+1-1)<<11)|(2<<4);	/* 8-bit, 1 stop, no parity; UART mode */
	intrenable(VectorCPIC+3, smcuintr, up, BUSUNKNOWN);
	smc->smcmr |= 3;	/* enable rx/tx */
}

static void
smcuintr(Ureg*, void *a)
{
	Uart *up;
	int events;

	up = a;
	events = up->smc->smce;
	eieio();
	up->smc->smce = events;
	uartintr(up, events&(BSY|RXB|TXB));
}

static void
sccustop(void *a)
{
	Uart *up;

	up = a;
	if(up->setup){
		scc2stop(up->scc);
		brgfree(up->brgc);
		up->brgc = -1;
		up->setup = 0;
	}
}
		
static void
sccsetup(Uart *up)
{
	IMM *io;
	SCC *scc;
	ulong bg;
	int i;

	scc = IOREGS(sccbase[up->x], SCC);
	if(up->x == 2){	/* BUG: 823FADS-specific */
		if(scc->gsmrl & (ENT|ENR))
			scc2stop(scc);
		scc->gsmrl &= ~(ENT|ENR);
		scc2claim(1, up->mode&SccIR? DisableIR: DisableRS232b, sccustop, scc);
	}
	up->scc = scc;
	if(up->brgc < 0){
		up->brgc = brgalloc();
		if(up->brgc < 0)
			error(Eio);
	}
	bg = baudgen(up->baud, 16) | BaudEnable;

	io = ioplock();
	io->papar |= SIBIT(12)|SIBIT(13);	/* enable TXD2 and RXD2 pins */
	io->padir &= ~(SIBIT(12)|SIBIT(13));
	if((up->mode & SccIR) == 0)
		io->paodr |= SIBIT(12);	/* TO DO: is this right? */
	else
		io->paodr &= ~(SIBIT(12)|SIBIT(13));	/* TO DO: is this right? */

	io->pbpar &= ~IBIT(18);	/* disable RTS2 */
	io->pbdir &= ~IBIT(18);
	if((up->mode&SccIR) == 0)
		io->pcpar |= SIBIT(14);	/* RTS2~ */
	else
		io->pcpar &= ~SIBIT(14);
	io->pcpar &= ~(SIBIT(9)|SIBIT(8));	/* CTS2~ and CD2~ */
	io->pcdir &= ~(SIBIT(14)|SIBIT(9)|SIBIT(8));
	if(FADS823 || up->mode)
		io->pcso &= ~(SIBIT(9)|SIBIT(8));	/* 82xFADS: force CTS and CD on */
	else
		io->pcso |= SIBIT(9)|SIBIT(8);
	eieio();

	io->brgc[up->brgc] = bg;
	iopunlock();

	sccnmsi(2, up->brgc, up->brgc);

	up->param = KADDR(sccparam[up->x]);
	uartsetbuf(up);

	cpmop(InitRxTx, up->cpmid, 0);

	/* SCC protocol parameters */
	if((up->mode & (SccAHDLC|SccHDLC)) == 0){
		Uartscc *sp;
		sp = (Uartscc*)up->param;
		sp->max_idl = 1;
		sp->brkcr = 1;
		sp->parec = 0;
		sp->frmec = 0;
		sp->nosec = 0;
		sp->brkec = 0;
		sp->brkln = 0;
		sp->brkec = 0;
		sp->uaddr1 = 0;
		sp->uaddr2 = 0;
		sp->toseq = 0;
		for(i=0; i<8; i++)
			sp->character[i] = 0x8000;
		sp->rccm = 0xC0FF;
		up->brkcr = &sp->brkcr;
		scc->irmode = 0;
		scc->dsr = ~0;
		scc->gsmrh = 1<<5;	/* 8-bit oriented receive fifo */
		scc->gsmrl = 0x28004;	/* UART mode */
	}else{
		UartAHDLC *hp;
		hp = (UartAHDLC*)up->param;
		hp->c_mask = 0x0000F0B8;
		hp->c_pres = 0x0000FFFF;
		if(up->mode & SccIR){
			hp->bof = 0xC0;
			hp->eof = 0xC1;
			//scc->dsr = 0xC0C0;
			scc->dsr = 0x7E7E;
		}else{
			hp->bof = 0x7E;
			hp->eof = 0x7E;
			scc->dsr = 0x7E7E;
		}
		hp->esc = 0x7D;
		hp->zero = 0;
		if(up->mode & SccHDLC)
			hp->rfthr = 1;
		else
			hp->rfthr = 0;	/* receive threshold of 1 doesn't work properly for Async HDLC */
		hp->txctl_tbl = 0;
		hp->rxctl_tbl = 0;
		if(up->mode & SccIR){
			/* low-speed infrared */
			hp->nof = 12-1;	/* 12 flags */
			scc->irsip = 0;
			scc->irmode = (2<<8) | 1;
//			m->bcsr[1] |= DisableIR;
//			microdelay(2);
//			m->bcsr[1] &= ~DisableIR;	/* put TFDS6000 xcvr in low-speed mode (see 4.9.2.1 in FADS manual) */
			if(up->loopback)
				scc->irmode = (3<<4)|1;	/* loopback */
		}else{
			scc->irmode = 0;
			hp->txctl_tbl = ~0;
			hp->rxctl_tbl = ~0;
			hp->nof = 1-1;	/* one opening flag */
		}
		up->brkcr = nil;
		scc->gsmrh = 1<<5;	/* 8-bit oriented receive fifo */
		if(up->mode & SccHDLC)
			scc->gsmrl = 0x28000;	/* HDLC */
		else
			scc->gsmrl = 0x28006;	/* async HDLC/IrDA */
	}
	scc->scce = ~0;	/* clear events */
	scc->sccm = TXE|BSY|RXF|TXB|RXB;	/* enable all interesting interrupts */
	intrenable(VectorCPIC+sccirq[up->x], sccuintr, up, BUSUNKNOWN);
	scc->psmr = 3<<12;	/* 8-bit, 1 stop, no parity; UART mode */
	if(up->loopback && (up->mode & SccIR) == 0)
		scc->gsmrl |= 1<<6;	/* internal loop back */
	scc->gsmrl |= ENT|ENR;	/* enable rx/tx */
	if(0){
		print("gsmrl=%8.8lux gsmrh=%8.8lux dsr=%4.4ux irmode=%4.4ux\n", scc->gsmrl, scc->gsmrh, scc->dsr, scc->irmode);
		for(i=0; i<sizeof(Uartscc); i+=4)
			print("%2.2ux %8.8lux\n", i, *(ulong*)((uchar*)up->param+i));
	}
}

static void
sccuintr(Ureg*, void *a)
{
	Uart *up;
	int events;

	up = a;
	if(/*(m->bcsr[1] & (DisableRS232b|DisableIR)) == (DisableRS232b|DisableIR) ||*/ up->scc == nil)
		return;
	events = up->scc->scce;
	eieio();
	up->scc->scce = events;
	if(up->enabled){
		if(0)
			print("#%ux|", events);
		uartintr(up, events);
	}
}

static void
uartsetbaud(Uart *p, int rate)
{
	if(rate <= 0 || p->brgc < 0)
		return;
	p->baud = rate;
	m->iomem->brgc[p->brgc] = baudgen(rate, 16) | BaudEnable;
}

static void
uartsetmode(Uart *p)
{
	int r, clen;

	ilock(&p->plock);
	clen = p->bpc;
	if(p->parity == 'e' || p->parity == 'o')
		clen++;
	clen++;	/* stop bit */
	if(p->stopb == 2)
		clen++;
	if(p->smc){
		r = p->smc->smcmr & 0x3F;	/* keep mode, enable bits */
		r |= (clen<<11);
		if(p->parity == 'e')
			r |= 3<<8;
		else if(p->parity == 'o')
			r |= 2<<8;
		if(p->stopb == 2)
			r |= 1<<10;
		eieio();
		p->smc->smcmr = r;
	}else if(p->scc && p->mode == 0){
		r = p->scc->psmr & 0x8FE0;	/* keep mode bits */
		r |= ((p->bpc-5)&3)<<12;
		if(p->parity == 'e')
			r |= (6<<2)|2;
		else if(p->parity == 'o')
			r |= (4<<2)|0;
		if(p->stopb == 2)
			r |= 1<<14;
		eieio();
		p->scc->psmr = r;
	}
	iunlock(&p->plock);
}

static void
uartparity(Uart *p, char type)
{
	ilock(&p->plock);
	p->parity = type;
	iunlock(&p->plock);
	uartsetmode(p);
}

/*
 *  set bits/character
 */
static void
uartbits(Uart *p, int bits)
{
	if(bits < 5 || bits > 14 || bits > 8 && p->scc)
		error(Ebadarg);

	ilock(&p->plock);
	p->bpc = bits;
	iunlock(&p->plock);
	uartsetmode(p);
}


/*
 *  toggle DTR
 */
static void
uartdtr(Uart *p, int n)
{
	if(p->scc == nil)
		return;	/* not possible */
	USED(n);	/* not possible on FADS */
}

/*
 *  toggle RTS
 */
static void
uartrts(Uart *p, int n)
{
	p->rts = n;
	if(p->scc == nil)
		return;	/* not possible */
	USED(n);	/* not possible on FADS */
}

/*
 *  send break
 */
static void
uartbreak(Uart *p, int ms)
{
	if(p->brkcr == nil)
		return;

	if(ms <= 0)
		ms = 200;

	if(waserror()){
		ilock(&p->plock);
		*p->brkcr = 1;
		cpmop(RestartTx, p->cpmid, 0);
		iunlock(&p->plock);
		nexterror();
	}
	ilock(&p->plock);
	*p->brkcr = ((p->baud/(p->bpc+2))*ms+500)/1000;
	cpmop(StopTx, p->cpmid, 0);
	iunlock(&p->plock);

	tsleep(&up->sleep, return0, 0, ms);

	poperror();
	ilock(&p->plock);
	*p->brkcr = 1;
	cpmop(RestartTx, p->cpmid, 0);
	iunlock(&p->plock);
}

/*
 *  modem flow control on/off (rts/cts)
 */
static void
uartmflow(Uart *p, int n)
{
	if(p->scc == nil)
		return;	/* not possible */
	if(n){
		p->modem = 1;
		/* enable status interrupts ... */
		p->scc->psmr |= 1<<15;	/* enable async flow control */
		p->cts = 1;
		/* could change maxidl */
	}else{
		p->modem = 0;
		/* stop status interrupts ... */
		p->scc->psmr &= ~(1<<15);
		p->cts = 1;
	}
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
void
uartenable(Uart *p)
{
	Uart **l;

	if(p->enabled)
		return;

	if(p->setup == 0){
		if(p->cpmid == SMC1ID || p->cpmid == SMC2ID)
			smcsetup(p);
		else
			sccsetup(p);
		p->setup = 1;
	}

	/*
 	 *  turn on interrupts
	 */
	if(p->smc){
		cpmop(RestartTx, p->cpmid, 0);
		p->smc->smcmr |= 3;
		p->smc->smcm = BSY|TXB|RXB;
		eieio();
	}else if(p->scc){
		cpmop(RestartTx, p->cpmid, 0);
		p->scc->gsmrl |= ENT|ENR;
		p->scc->sccm = BSY|TXB|RXB;
		eieio();
	}

	/*
	 *  turn on DTR and RTS
	 */
	uartdtr(p, 1);
	uartrts(p, 1);

	/*
	 *  assume we can send
	 */
	p->cts = 1;
	p->blocked = 0;

	/*
	 *  set baud rate to the last used
	 */
	uartsetbaud(p, p->baud);

	lock(&uartalloc);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p)
			break;
	}
	if(*l == 0){
		p->elist = uartalloc.elist;
		uartalloc.elist = p;
	}
	p->enabled = 1;
	unlock(&uartalloc);
	p->cts = 1;
	p->blocked = 0;
	p->xonoff = 0;
	p->enabled = 1;
}

/*
 *  turn off a port's interrupts.  reset DTR and RTS
 */
void
uartdisable(Uart *p)
{
	Uart **l;

	/*
 	 *  turn off interrpts
	 */
	if(p->smc){
		cpmop(StopTx, p->cpmid, 0);
		cpmop(CloseRxBD, p->cpmid, 0);
		delay(1);
		p->smc->smcmr &= ~3;
		eieio();
		p->smc->smcm = 0;
	}else if(p->scc){
		cpmop(GracefulStopTx, p->cpmid, 0);
		delay(1);
		cpmop(CloseRxBD, p->cpmid, 0);
		p->scc->gsmrl &= ~(ENT|ENR);
		eieio();
		p->scc->sccm = 0;
	}

	/*
	 *  revert to default settings
	 */
	p->bpc = 8;
	p->parity = 0;
	p->stopb = 0;

	/*
	 *  turn off DTR, RTS, hardware flow control & fifo's
	 */
	uartdtr(p, 0);
	uartrts(p, 0);
	uartmflow(p, 0);
	p->xonoff = p->blocked = 0;

	lock(&uartalloc);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p){
			*l = p->elist;
			break;
		}
	}
	p->enabled = 0;
	unlock(&uartalloc);
}

/*
 *  set the next output buffer going
 */
static void
txstart(Uart *p)
{
	Block *b;
	int n, flags;


	if(!p->cts || p->blocked || p->txb->status & BDReady)
		return;

	if((b = p->outb) == nil){
		if((b = qget(p->oq)) == nil)
			return;
		if(p->mode & SccPPP &&
		   p->mode & SccAHDLC &&
		   BLEN(b) >= 8){	/* strip framing data */
			UartAHDLC *hp;
			hp = (UartAHDLC*)p->param;
			if(hp != nil && (p->mode & SccIR) == 0){
				hp->txctl_tbl = nhgetl(b->rp);
				hp->rxctl_tbl = nhgetl(b->rp+4);
			}
			b->rp += 8;
			if(0)
				print("tx #%lux rx #%lux\n", hp->txctl_tbl, hp->rxctl_tbl);
		}
	}
	n = BLEN(b);
//	if(n <= 0)
//		print("txstart: 0\n");
	if(p->bpc > 8){
		/* half-word alignment and length if chars are long */
		if(PADDR(b->rp)&1){	/* must be even if chars are long */
			memmove(b->base, b->rp, n);
			b->rp = b->base;
			b->wp = b->rp+n;
		}
		if(n & 1)
			n++;
	}
	dcflush(b->rp, n);
	p->outb = b;
	if(n > 0xFFFF)
		n = 0xFFFE;
	if(p->mode & SccHDLC)
		flags = BDLast | TxTC;
	else if(p->mode)
		flags = BDLast;
	else
		flags = 0;
	p->txb->addr = PADDR(b->rp);
	p->txb->length = n;
	eieio();
	p->txb->status = (p->txb->status & BDWrap) | flags | BDReady | BDInt;
	eieio();
}

/*
 *  (re)start output
 */
static void
uartkick(void *arg)
{
	Uart *p = arg;
	ilock(&p->plock);
	if(p->outb == nil)
		txstart(p);
	iunlock(&p->plock);

	if(predawn)
		uartwait();
}

/*
 *  restart input if its off
 */
static void
uartflow(void *arg)
{
	Uart *p = arg;
	if(p->modem)
		uartrts(p, 1);
}

static void
uartsetup(int x, int cpmid, char *name)
{
	Uart *p;

	if(nuart >= Nuart)
		return;

	p = xalloc(sizeof(Uart));
	uart[nuart] = p;
	strcpy(p->name, name);
	p->dev = nuart;
	nuart++;
	p->x = x;
	p->cpmid = cpmid;
	p->brgc = -1;
	p->mode = 0;

	/*
	 *  set rate to 9600 baud.
	 *  8 bits/character.
	 *  1 stop bit.
	 *  interrupts enabled.
	 */
	p->bpc = 8;
	p->parity = 0;
	p->baud = 9600;

	p->iq = qopen(4*1024, 0, uartflow, p);
	p->oq = qopen(4*1024, 0, uartkick, p);
}

/*
 *  called by main() to configure a duart port as a console or a mouse
 */
void
uartspecial(int port, int baud, Queue **in, Queue **out, int (*putc)(Queue*, int))
{
	Uart *p = uart[port];

	uartenable(p);
	if(baud)
		uartsetbaud(p, baud);
	p->putc = putc;
	if(in)
		*in = p->iq;
	if(out)
		*out = p->oq;
	p->opens++;
}

int lastc = 'x';

static int
uartinput(Uart *p, BD *bd)
{
	int ch, dokick, i, l;
	uchar *bp;

	dokick = 0;
	if(bd->status & RxFR)
		p->frame++;
	if(bd->status & RxOV)
		p->overrun++;
	l = bd->length;
	if(bd->status & RxPR){
		p->perror++;
		l--;	/* it's the last character */
	}
	bp = KADDR(bd->addr);
	if(p->xonoff || p->putc){
		for(i=0; i<l; i++){
			ch = bp[i];
			if(p->xonoff){
				if(ch == CTLS){
					p->blocked = 1;
					cpmop(StopTx, p->cpmid, 0);
				}else if (ch == CTLQ){
					p->blocked = 0;
					dokick = 1;
				}
				/* BUG? should discard on/off char? */
			}
			if(p->putc) {
				(*p->putc)(p->iq, ch);
				lastc = ch;
			}
		}
	}
	if(p->putc == nil && l > 0)
		qproduce(p->iq, bp, l);
	return dokick;
}

static void
framedinput(Uart *p, BD *bd)
{
	Block *pkt;
	int l;

	pkt = p->partial;
	p->partial = nil;
	if(bd->status & RxOV){
		p->overrun++;
		goto Discard;
	}
	if(bd->status & (RxAB|RxCR|RxCD|RxLG|RxNO|RxDE|RxBOF|RxBRK)){
		if(bd->status & RxCR)
			p->crcerr++;
		else
			p->frame++;
		goto Discard;
	}
	if(pkt == nil){
		pkt = iallocb(1500);	/* TO DO: allocate less if possible */
		if(pkt == nil)
			return;
	}
	l = bd->length;
	if(bd->status & BDLast)
		l -= BLEN(pkt);	/* last one gives size of entire frame */
	if(l > 0){
		if(pkt->wp+l > pkt->lim)
			goto Discard;
		memmove(pkt->wp, KADDR(bd->addr), l);
		pkt->wp += l;
	}
	if(0)
		print("#%ux|", bd->status);
	if(bd->status & BDLast){
		if(p->mode & (SccHDLC|SccAHDLC)){
			if(BLEN(pkt) <= 2){
				p->frame++;
				goto Discard;
			}
			pkt->wp -= 2;	/* strip CRC */
		}
		qpass(p->iq, pkt);
	}else
		p->partial = pkt;
	return;

Discard:
	if(pkt != nil)
		freeb(pkt);
}

/*
 *  handle an interrupt to a single uart
 */
static void
uartintr(Uart *p, int events)
{
	int dokick;
	BD *bd;
	Block *b;
	if(events & BSY)
		p->overrun++;
	p->interrupts++;
	dokick = 0;
	while(p->rxb != nil && ((bd = &p->rxb[p->rdrx])->status & BDEmpty) == 0){
		if(p->mode)
			framedinput(p, bd);
		else if(uartinput(p, bd))
			dokick = 1;
		dcflush(KADDR(bd->addr), bd->length);
		eieio();
		bd->status = (bd->status & BDWrap) | BDEmpty|BDInt;
		p->rdrx ^= 1;
	}
	if((bd = p->txb) != nil){
		if((bd->status & BDReady) == 0){
			ilock(&p->plock);
			if((b = p->outb) != nil){
				b->rp += bd->length;
				if(b->rp >= b->wp){
					p->outb = nil;
					freeb(b);
				}
			}
			txstart(p);
			iunlock(&p->plock);
		}
	}
	USED(dokick);
#ifdef XXX
	eieio();
	/* TO DO: modem status isn't available on 82xFADS */
	if(dokick && p->cts && !p->blocked){
		if(p->outb == nil){
			ilock(&p->plock);
			txstart(p);
			iunlock(&p->plock);
		}
		cpmop(RestartTx, p->cpmid, 0);
	} else 
#endif

	if (events & TXE)
		cpmop(RestartTx, p->cpmid, 0);
}

/*
 * used to ensure uart console output when debugging
 */
void
uartwait(void)
{
	Uart *p = uart[0];
	int s;

	while(p && p->outb){
		s = splhi();
		if((p->txb->status & BDReady) == 0){
			p->blocked = 0;
			p->cts = 1;
			if(p->cpmid == SMC1ID || p->cpmid == SMC2ID)
				smcuintr(nil, p);
			else
				sccuintr(nil, p);
		}
		splx(s);
	}
}

static Dirtab *uartdir;
static int ndir;

static void
setlength(int i)
{
	Uart *p;

	if(i > 0){
		p = uart[i];
		if(p && p->opens && p->iq)
			uartdir[3*i].length = qlen(p->iq);
	} else for(i = 0; i < nuart; i++){
		p = uart[i];
		if(p && p->opens && p->iq)
			uartdir[3*i].length = qlen(p->iq);
	}
		
}

void
uartinstall(void)
{
	static int already;

	if(already)
		return;
	already = 1;
	uartsetup(1, SMC1ID, "eia0");
/*
	int port;
	char *p, *q;
	if((p = getconf("console")) || (p = "0")){
		if(USESMC2)
			port = strtol(p, &q, 0);
		else
			port = 0;
		if(p != q && (port == 0 || port == 1))
			uartspecial(port, 9600, &kbdq, &printq, kbdcr2nl);
	}
*/
	uartspecial(0, 9600, &kbdq, &printq, kbdcr2nl);

//	if(USESMC2)
//		uartsetup(2, SMC2ID, "eia1");
//	if(USESCC2)
//		uartsetup(2, SCC2ID, "eia2");
}

/*
 *  all uarts must be uartsetup() by this point or inside of uartinstall()
 */
static void
uartreset(void)
{
	int i;
	Dirtab *dp;

	uartinstall();	/* architecture specific */

	ndir = 4*nuart;
	uartdir = xalloc(ndir * sizeof(Dirtab));
	dp = uartdir;
	for(i = 0; i < nuart; i++){
		/* 3 directory entries per port */
		strcpy(dp->name, uart[i]->name);
		dp->qid.path = NETQID(i, Ndataqid);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "%sctl", uart[i]->name);
		dp->qid.path = NETQID(i, Nctlqid);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "%sstat", uart[i]->name);
		dp->qid.path = NETQID(i, Nstatqid);
		dp->perm = 0444;
		dp++;
		sprint(dp->name, "%smode", uart[i]->name);
		dp->qid.path = NETQID(i, Ntypeqid);
		dp->perm = 0660;
		dp++;
	}
}

static Chan*
uartattach(char *spec)
{
	return devattach('t', spec);
}

static int
uartwalk(Chan *c, char *name)
{
	return devwalk(c, name, uartdir, ndir, devgen);
}

static void
uartstat(Chan *c, char *dp)
{
	if(NETTYPE(c->qid.path) == Ndataqid)
		setlength(NETID(c->qid.path));
	devstat(c, dp, uartdir, ndir, devgen);
}

static Chan*
uartopen(Chan *c, int omode)
{
	Uart *p;

	c = devopen(c, omode, uartdir, ndir, devgen);

	switch(NETTYPE(c->qid.path)){
	case Nctlqid:
	case Ndataqid:
		p = uart[NETID(c->qid.path)];
		qlock(p);
		if(p->opens++ == 0){
			uartenable(p);
			qreopen(p->iq);
			qreopen(p->oq);
		}
		qunlock(p);
		break;
	}

	return c;
}

static void
uartclose(Chan *c)
{
	Uart *p;

	if(c->qid.path & CHDIR)
		return;
	if((c->flag & COPEN) == 0)
		return;
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
	case Nctlqid:
		p = uart[NETID(c->qid.path)];
		qlock(p);
		if(--(p->opens) == 0){
			uartdisable(p);
			qclose(p->iq);
			qclose(p->oq);
		}
		qunlock(p);
		break;
	}
}

static long
uartstatus(Chan*, Uart *p, void *buf, long n, long offset)
{
	IMM *io;
	char str[256];

	sprint(str, "opens %d ferr %lud oerr %lud crcerr %lud baud %ud perr %lud intr %lud lastc = %c", p->opens,
		p->frame, p->overrun, p->crcerr, p->baud, p->perror, p->interrupts, lastc);
	/* TO DO: cts, dsr, ring, dcd, dtr, rts aren't all available on 82xFADS */
	io = m->iomem;
	if(p->scc){
		if((io->pcdat & SIBIT(9)) == 0)
			strcat(str, " cts");
		if((io->pcdat & SIBIT(8)) == 0)
			strcat(str, " dcd");
		if((io->pbdat & IBIT(22)) == 0)
			strcat(str, " dtr");
	}else if(p->smc){
		if((io->pbdat & IBIT(23)) == 0)
			strcat(str, " dtr");
	}
	strcat(str, "\n");
	return readstr(offset, buf, n, str);
}

static long
uartread(Chan *c, void *buf, long n, vlong offset)
{
	Uart *p;

	if(c->qid.path & CHDIR){
		setlength(-1);
		return devdirread(c, buf, n, uartdir, ndir, devgen);
	}

	p = uart[NETID(c->qid.path)];
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qread(p->iq, buf, n);
	case Nctlqid:
		return readnum(offset, buf, n, NETID(c->qid.path), NUMSIZE);
	case Nstatqid:
		return uartstatus(c, p, buf, n, offset);
	case Ntypeqid:
		return readnum(offset, buf, n, p->mode, NUMSIZE);
	}

	return 0;
}

static Block*
uartbread(Chan *c, long n, ulong offset)
{
	if(c->qid.path & CHDIR || NETTYPE(c->qid.path) != Ndataqid)
		return devbread(c, n, offset);
	return qbread(uart[NETID(c->qid.path)]->iq, n);
}

static void
uartctl(Uart *p, char *cmd)
{
	int i, n;

	/* let output drain for a while */
	for(i = 0; i < 16 && qlen(p->oq); i++)
		tsleep(&p->r, (int (*)(void*))qlen, p->oq, 125);

	if(strncmp(cmd, "break", 5) == 0){
		uartbreak(p, 0);
		return;
	}
		
	n = atoi(cmd+1);
	switch(*cmd){
	case 'B':
	case 'b':
		uartsetbaud(p, n);
		break;
	case 'D':
	case 'd':
		uartdtr(p, n);
		break;
	case 'f':
	case 'F':
		qflush(p->oq);
		break;
	case 'H':
	case 'h':
		qhangup(p->iq, 0);
		qhangup(p->oq, 0);
		break;
	case 'L':
	case 'l':
		uartbits(p, n);
		break;
	case 'm':
	case 'M':
		uartmflow(p, n);
		break;
	case 'n':
	case 'N':
		qnoblock(p->oq, n);
		break;
	case 'P':
	case 'p':
		uartparity(p, *(cmd+1));
		break;
	case 'K':
	case 'k':
		uartbreak(p, n);
		break;
	case 'R':
	case 'r':
		uartrts(p, n);
		break;
	case 'Q':
	case 'q':
		qsetlimit(p->iq, n);
		qsetlimit(p->oq, n);
		break;
	case 'W':
	case 'w':
		/* obsolete */
		break;
	case 'X':
	case 'x':
		p->xonoff = n;
		break;
	case 'Z':
	case 'z':
		p->loopback = n;
		break;
	}
}

static long
uartwrite(Chan *c, void *buf, long n, vlong offset)
{
	Uart *p;
	char cmd[32];
	int m, inuse;

	USED(offset);

	if(c->qid.path & CHDIR)
		error(Eperm);

	p = uart[NETID(c->qid.path)];

	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qwrite(p->oq, buf, n);
	case Nctlqid:
		if(n >= sizeof(cmd))
			n = sizeof(cmd)-1;
		memmove(cmd, buf, n);
		cmd[n] = 0;
		uartctl(p, cmd);
		return n;
	case Ntypeqid:
		if(p->smc || p->putc)
			error(Ebadarg);
		if(n >= sizeof(cmd))
			n = sizeof(cmd)-1;
		memmove(cmd, buf, n);
		cmd[n] = 0;
		m = strtoul(cmd, nil, 0);
		inuse = 0;
		qlock(p);
		if(p->opens == 0){
			p->mode = m & 0x7F;
			p->loopback = (m&0x80)!=0;
			p->setup = 0;
		}else
			inuse = 1;
		qunlock(p);
		if(inuse)
			error(Einuse);
		return n;
	}
}

static long
uartbwrite(Chan *c, Block *bp, ulong offset)
{
	/* TO DO: better */
	return devbwrite(c, bp, offset);
}

static void
uartwstat(Chan *c, char *dp)
{
	Dir d;
	Dirtab *dt;

	if(!iseve())
		error(Eperm);
	if(CHDIR & c->qid.path)
		error(Eperm);
	if(NETTYPE(c->qid.path) == Nstatqid)
		error(Eperm);

	dt = &uartdir[3 * NETID(c->qid.path)];
	convM2D(dp, &d);
	d.mode &= 0666;
	dt[0].perm = dt[1].perm = d.mode;
}

Dev uartdevtab = {
	't',
	"uart",

	uartreset,
	devinit,
	uartattach,
	devclone,
	uartwalk,
	uartstat,
	uartopen,
	devcreate,
	uartclose,
	uartread,
	uartbread,
	uartwrite,
	uartbwrite,
	devremove,
	uartwstat,
};
