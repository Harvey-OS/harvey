#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

typedef struct SPIparam SPIparam;
struct SPIparam {
	ushort	rbase;
	ushort	tbase;
	uchar	rfcr;
	uchar	tfcr;
	ushort	mrblr;
	ulong	rstate;
	ulong	rptr;
	ushort	rbptr;
	ushort	rcnt;
	ulong	rtmp;
	ulong	tstate;
	ulong	tptr;
	ushort	tbptr;
	ushort	tcnt;
	ulong	ttmp;
};

enum {
	Nrdre		= 2,	/* receive descriptor ring entries */
	Ntdre		= 2,	/* transmit descriptor ring entries */

	Rbsize		= 8,		/* ring buffer size (+3 for PID+CRC16) */
	Bufsize		= (Rbsize+7)&~7,	/* aligned */
};

enum {
	/* spi-specific BD flags */
	BDContin=	1<<9,	/* continuous mode */
	BDrxov=		1<<1,	/* overrun */
	BDtxun=		1<<1,	/* underflow */
	BDme=		1<<0,	/* multimaster error */
	BDrxerr=		BDrxov|BDme,
	BDtxerr=		BDtxun|BDme,

	/* spmod */
	MLoop=	1<<14,	/* loopback mode */
	MClockInv= 1<<13,	/* inactive state of SPICLK is high */
	MClockPhs= 1<<12,	/* SPCLK starts toggling at beginning of transfer */
	MDiv16=	1<<11,	/* use BRGCLK/16 as input to SPI baud rate */
	MRev=	1<<10,	/* normal operation */
	MMaster=	1<<9,
	MSlave=	0<<9,
	MEnable=	1<<8,
	/* LEN, PS fields */

	/* spie */
	MME =	1<<5,
	TXE =	1<<4,
	RES =	1<<3,
	BSY =	1<<2,
	TXB =	1<<1,
	RXB =	1<<0,
};

/*
 * software structures
 */
typedef struct Ctlr Ctlr;

struct Ctlr {
	QLock;
	int	init;
	SPI*	spi;
	SPIparam*	sp;

	int	rxintr;
	Rendez	ir;
	Ring;
};

static	Ctlr	spictlr[1];

static	void	interrupt(Ureg*, void*);

/*
 * test driver for SPI, host mode
 */
void
spireset(void)
{
	IMM *io;
	SPI *spi;
	SPIparam *sp;
	Ctlr *ctlr;

	io = m->iomem;
	sp = (SPIparam*)cpmparam(SPIP, sizeof(*sp));
	memset(sp, 0, sizeof(*sp));
	if(sp == nil){
		print("SPI: can't allocate new parameter memory\n");
		return;
	}

	spi = (SPI*)((ulong)m->iomem+0xAA0);
	ctlr = spictlr;

	/* step 1: select port pins */
	io->pbdir |= IBIT(30)|IBIT(29)|IBIT(28);
	io->pbpar |= IBIT(30)|IBIT(29)|IBIT(28);	/* SPICLK, SPIMOSI, SPIMISO */

	/* step 2: set CS pin - also disables SPISEL */
	io->pbpar &= ~(IBIT(31));
	io->pbdir |= IBIT(31);
	io->pbodr &= ~(IBIT(31));
	io->pbdat &= ~(IBIT(31));

	ctlr->spi = spi;
	ctlr->sp = sp;

	if(ioringinit(ctlr, Nrdre, Ntdre, Bufsize)<0)
		panic("spireset: ioringinit");

	/* step 3: configure rbase and tbase */
	sp->rbase = PADDR(ctlr->rdr);
	sp->tbase = PADDR(ctlr->tdr);

	/* step 4: do not issue InitRxTx command - due to microcode bug */

	/* step 5: */
	io->sdcr = 1;

	/* step 6: */
	sp->rfcr = 0x10;
	sp->tfcr = 0x10;

	/* step 7: */
	sp->mrblr = Bufsize;

	/* step 7.5: init other params because of bug in microcode */
	sp->rstate = 0;
	sp->rbptr = sp->rbase;
	sp->rcnt = 0;
	sp->tstate = 0;
	sp->tbptr = sp->tbase;
	sp->tcnt = 0;

	/* step 8-9: done by ioringinit */

	/* step 10: clear events */
	spi->spie = ~0;	

	/* step 11-12: enable interrupts  */
	intrenable(VectorCPIC+0x05, interrupt, spictlr, BUSUNKNOWN);
	spi->spim = ~0;

	/* step 13: enable in master mode, 8 bit chacters, slow clock */
	spi->spmode = MMaster|MRev|MDiv16|MEnable|(0x7<<4)|0xf;

}

static void
interrupt(Ureg*, void *arg)
{
	int events;
	Ctlr *ctlr;
	SPI *spi;

	ctlr = arg;
	spi = ctlr->spi;
	events = spi->spie;
	spi->spie = events;
//	print("SPI#%x\n", events);
}

void
spdump(SPIparam *sp)
{
	print("rbase = %ux\n", sp->rbase);
	print("tbase = %ux\n", sp->tbase);
	print("rfcr = %ux\n", sp->rfcr);
	print("tfcr = %ux\n", sp->tfcr);
	print("mrblr = %ux\n", sp->mrblr);
	print("rstate = %lux\n", sp->rstate);
	print("rptr = %lux\n", sp->rptr);
	print("rbptr = %ux\n", sp->rbptr);
	print("rcnt = %ux\n", sp->rcnt);
	print("rtmp = %lux\n", sp->rtmp);
	print("tstate = %lux\n", sp->tstate);
	print("tptr = %lux\n", sp->tptr);
	print("tbptr = %ux\n", sp->tbptr);
	print("tcnt = %ux\n", sp->tcnt);
	print("ttmp = %lux\n", sp->ttmp);
}

void
spitest(void)
{
	BD *dre;
	Block *b;
	int len;
	Ctlr *ctlr;
	ulong status;
	uchar buf[2];

	ctlr = spictlr;

	dre = &ctlr->tdr[ctlr->tdrh];
	if(dre->status & BDReady)
		panic("spitest: txstart");

print("dre->status %ux %ux\n", dre[0].status, dre[1].status);	
	b = allocb(4);
	b->wp[0] = 0xc0;
	b->wp[1] = 0x00;
	b->wp[2] = 0x00;
	b->wp[3] = 0x00;
	b->wp += 4;
	len = BLEN(b);
print("len = %d\n", len);
	dcflush(b->rp, len);
	if(ctlr->txb[ctlr->tdrh] != nil)
		panic("scc/ether: txstart");
	ctlr->txb[ctlr->tdrh] = b;
	dre->addr = PADDR(b->rp);
print("addr = %lux\n", PADDR(b->rp));
	dre->length = len;
	dre->status = (dre->status & BDWrap) | BDReady|BDInt|BDLast;
spdump(ctlr->sp);
m->iomem->pbdat |= IBIT(31);
microdelay(1);
	ctlr->spi->spcom = 1<<7;	/* transmit now */
print("cpcom = %p\n", &ctlr->spi->spcom);
spdump(ctlr->sp);
	eieio();
	ctlr->ntq++;
	ctlr->tdrh = NEXT(ctlr->tdrh, Ntdre);
print("going into loop %ux %ux\n", dre->status, ctlr->spi->spcom);
	while(dre->status&BDReady) {
print("ctrl->spi->spim = %ux %ux\n", ctlr->spi->spie, dre->status);
spdump(ctlr->sp);
		delay(10000);
	}
delay(100);
	dre = &ctlr->rdr[ctlr->rdrx];
	status = dre->status;
	len = dre->length;
print("%d status = %lux len=%d\n", ctlr->rdrx, status, len);
	b = iallocb(len);
	memmove(b->wp, KADDR(dre->addr), len);
	b->wp += len;
print("%ux %ux %ux %ux\n", b->rp[0], b->rp[1], b->rp[2], b->rp[3]);

/* get the data out of the receiving block
   the data begins at the 13th bit, and ends at the (13+16)th bit
*/
	buf[0]=b->rp[1]<<4 | b->rp[2]>>4;
	buf[1]=b->rp[2]<<4 | b->rp[3]>>4;

print("the value of address 0 in EEPROM is %ux %ux\n",buf[0],buf[1]);
	dcflush(KADDR(dre->addr), len);
	dre->status = (status & BDWrap) | BDEmpty | BDInt;
	ctlr->rdrx = NEXT(ctlr->rdrx, Nrdre);
m->iomem->pbdat &= ~(IBIT(31));
microdelay(1);
}


void
spiinit(void)
{
	spireset();
//	spitest();
}

void
spicmdsend(Block *b, BD *dre, Ctlr *ctlr, int CSdown)
{   
	int len;

	if(dre->status & BDReady)
		panic("spicmdsend: txstart");
	
	len = BLEN(b);
	dcflush(b->rp, len);

	ctlr->txb[ctlr->tdrh] = b;
	dre->addr = PADDR(b->rp);
	dre->length = len;
	dre->status = (dre->status & BDWrap) | BDReady|BDInt|BDLast;
	m->iomem->pbdat |= IBIT(31);

	ctlr->spi->spcom = 1<<7;	/* transmit now */

	//eieio();
	ctlr->ntq++;
	ctlr->tdrh = NEXT(ctlr->tdrh, Ntdre);

	delay(1);

	while(dre->status&BDReady) microdelay(10);

	if(CSdown) m->iomem->pbdat &= ~(IBIT(31));

}


void
spiread(ulong addr,uchar * buf)
{

  	BD *dre;
	Block *b;
	int len;
	Ctlr *ctlr;
	ushort status;
  
	ctlr = spictlr;

	dre = &ctlr->tdr[ctlr->tdrh];

	b = allocb(4);
	b->wp[0] = 0xc0 | addr>>3;
	b->wp[1] = addr<<5;
	b->wp[2] = 0x00;
	b->wp[3] = 0x00;
	b->wp += 4;

	spicmdsend(b, dre, ctlr, 0);

	//delay(10);
	
	dre = &ctlr->rdr[ctlr->rdrx];
	status = dre->status;
	len = dre->length;
	
	while(dre->status&BDEmpty) microdelay(10);

	b = iallocb(len);
	memmove(b->wp, KADDR(dre->addr), len);
	b->wp += len;


/* get the data out of the receiving block
   the data begins at the 13th bit, and ends at the (13+16)th bit
*/
	buf[0]=b->rp[1]<<4 | b->rp[2]>>4;
	buf[1]=b->rp[2]<<4 | b->rp[3]>>4;

//    print("read out %ux %ux\n", buf[0],buf[1]);
	dcflush(KADDR(dre->addr), len);
	dre->status = (status & BDWrap) | BDEmpty | BDInt;
	ctlr->rdrx = NEXT(ctlr->rdrx, Nrdre);
	freeb(b);
	m->iomem->pbdat &= ~(IBIT(31));
	microdelay(5);

	return;
}

void
spienwrite(void)
{
  	BD *dre;
	Block *b;
	Ctlr *ctlr;
	int len;

	ctlr = spictlr;

	b = allocb(4);
	b->wp[0] = 0x98;
	b->wp[1] = 0x00;
	b->wp[2] = 0x00;
	b->wp[3] = 0x00;
	b->wp += 4;

	dre = &ctlr->tdr[ctlr->tdrh];
	spicmdsend(b,dre,ctlr,1);

	dre=&ctlr->rdr[ctlr->rdrx];
	len=dre->length;
	dcflush(KADDR(dre->addr),len);
	dre->status=(dre->status&BDWrap)|BDEmpty|BDInt;
	ctlr->rdrx=NEXT(ctlr->rdrx,Nrdre);

	freeb(b);
	microdelay(5);
	return;
}

void
spidiswrite(void)
{
  	BD *dre;
	Block *b;
	Ctlr *ctlr;
	int len;

	ctlr = spictlr;

	dre = &ctlr->tdr[ctlr->tdrh];

	b = allocb(4);
	b->wp[0] = 0x80;
	b->wp[1] = 0x00;
	b->wp[2] = 0x00;
	b->wp[3] = 0x00;
	b->wp += 4;

	spicmdsend(b,dre,ctlr,1);

	dre=&ctlr->rdr[ctlr->rdrx];
	len=dre->length;
	dcflush(KADDR(dre->addr),len);
	dre->status=(dre->status&BDWrap)|BDEmpty|BDInt;
	ctlr->rdrx=NEXT(ctlr->rdrx,Nrdre);

	freeb(b);
	microdelay(5);
	return;
}

void
spiwrite(ulong addr, uchar *buf)
{
  	BD *dre;
	Block *b;
	Ctlr *ctlr;
	int len;
  
	ctlr = spictlr;

	dre = &ctlr->tdr[ctlr->tdrh];

	b = allocb(4);
	b->wp[0] = 0x05 ;
	b->wp[1] = 0x7f & addr;
	b->wp+=2;
	memmove((uchar*)b->wp,buf,2);
	b->wp += 2;

	spicmdsend(b,dre,ctlr,1);

	dre=&ctlr->rdr[ctlr->rdrx];
	len=dre->length;
	dcflush(KADDR(dre->addr),len);
	dre->status=(dre->status&BDWrap)|BDEmpty|BDInt;
	ctlr->rdrx=NEXT(ctlr->rdrx,Nrdre);

	freeb(b);
	delay(15);
	//sleep(15); //sleep for 15ms 
	return;
}

void
spiwriteall(uchar *buf)
{
  	BD *dre;
	Block *b;
	Ctlr *ctlr;
	int len;

	ctlr = spictlr;

	dre = &ctlr->tdr[ctlr->tdrh];
	
	b = allocb(4);
	b->wp[0] = 0x04;
	b->wp[1] = 0x40;
	b->wp+=2;
	memmove((uchar*)b->wp,buf,2);
	//b->wp[2]=0xf0;
	//b->wp[3]=0xf0;
	b->wp += 2;

	spicmdsend(b,dre,ctlr,1);

	dre=&ctlr->rdr[ctlr->rdrx];
	len=dre->length;
	dcflush(KADDR(dre->addr),len);
	dre->status=(dre->status&BDWrap)|BDEmpty|BDInt;
	ctlr->rdrx=NEXT(ctlr->rdrx,Nrdre);

	freeb(b);
	delay(15);
	//sleep(15000); //sleep for 15ms 
	print("leave spiwriteall\n");
	return;
}

void
spierase(ulong addr)
{
  	BD *dre;
	Block *b;
	Ctlr *ctlr;
	int len;

	ctlr = spictlr;

	b = allocb(2);
	b->wp[0] = 0x07;
	b->wp[1] = addr&0x7f;
	b->wp+=2;

	dre = &ctlr->tdr[ctlr->tdrh];
	spicmdsend(b,dre,ctlr,1);

	dre=&ctlr->rdr[ctlr->rdrx];
	len=dre->length;
	dcflush(KADDR(dre->addr),len);
	dre->status=(dre->status&BDWrap)|BDEmpty|BDInt;
	ctlr->rdrx=NEXT(ctlr->rdrx,Nrdre);

	freeb(b);
	delay(15);
	return;
}

void
spieraseall(void)
{
  	BD *dre;
	Block *b;
	Ctlr *ctlr;
	int len;

	ctlr = spictlr;

	b = allocb(2);
	b->wp[0] = 0x07;
	b->wp[1] = 0x80;
	b->wp+=2;

	dre = &ctlr->tdr[ctlr->tdrh];
	spicmdsend(b,dre,ctlr,1);

	dre=&ctlr->rdr[ctlr->rdrx];
	len=dre->length;
	dcflush(KADDR(dre->addr),len);
	dre->status=(dre->status&BDWrap)|BDEmpty|BDInt;
	ctlr->rdrx=NEXT(ctlr->rdrx,Nrdre);

	freeb(b);
	delay(15);
	return;
}
