/*
 * PolarFire SDCard / eMMC host interface
 *
 *	host controller with 1 slot, multiplexed between SDCard and eMMC
 *	device assignment:
 *		sdM0 - external SDcard socket
 *		sdM1 - internal eMMC storage
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "riscv.h"
#include "../port/sd.h"

#define WR(reg, val)	r[reg] = val

enum {
	Debug		= 0,

	Maxunit		= 1,		/* number of slots */
	Maxdev		= 2,		/* number of devices */
	Extfreq		= 20000000,	/* guess external clock frequency */
	Initfreq	= 400000,	/* initialisation frequency for MMC */
	SDfreq		= 25000000,	/* standard SD frequency */
	DTO		= 14,		/* data timeout exponent (guesswork) */

	MMCSelect	= 7,		/* mmc/sd card select command */
	Setbuswidth	= 6,		/* mmc/sd set bus width command */
	SendExtendedCSD = 8,	/* mmc send extended CSD command */
	Voltageswitch = 11,		/* md/sdio switch to 1.8V */
	IORWdirect = 52,		/* sdio read/write direct command */
	IORWextended = 53,		/* sdio read/write extended command */
};

/*
 * map unit number to sdio select register setting
 */
static uchar selunit[] = {1, 0};

#define HRS(n)	(n)
#define SRS(n)	((0x200>>2) + (n))
enum {
	/* Host controller registers */
	Geninfo			= HRS(0),
	AXIErr			= HRS(3),

	/* Slot registers */
	Blksizecnt		= SRS(1),
	Arg1			= SRS(2),
	Cmdtm			= SRS(3),
	Resp0			= SRS(4),
	Resp1			= SRS(5),
	Resp2			= SRS(6),
	Resp3			= SRS(7),
	Data			= SRS(8),
	Status			= SRS(9),
	Control0		= SRS(10),
	Control1		= SRS(11),
	Interrupt		= SRS(12),
	Irptmask		= SRS(13),
	Irpten			= SRS(14),
	Control2		= SRS(15),
	Capability		= SRS(16),
	Dmadesc			= SRS(22),

	/* Geninfo */
	S0ddr			= 1<<28,
	S0mmc8			= 1<<24,
	S0avail			= 1<<16,
	S1avail			= 1<<17,
	S2avail			= 1<<18,
	S3avail			= 1<<19,
	Reset			= 1<<0,

	/* Control0 */
	Busvoltage		= 7<<9,
		V1_8		= 5<<9,
		V3_0		= 6<<9,
		V3_3		= 7<<9,
	Buspower		= 1<<8,
	Dwidth8			= 1<<5,
	Dmaselect		= 3<<3,
		DmaSDMA		= 0<<3,
		DmaADMA1	= 1<<3,
		DmaADMA2	= 2<<3,
	Hispeed			= 1<<2,
	Dwidth4			= 1<<1,
	Dwidth1			= 0<<1,
	LED				= 1<<0,

	/* Control2 */
	Sig1_8			= 1<<19,

	/* Control1 */
	Srstdata		= 1<<26,	/* reset data circuit */
	Srstcmd			= 1<<25,	/* reset command circuit */
	Srsthc			= 1<<24,	/* reset complete host controller */
	Datatoshift		= 16,		/* data timeout unit exponent */
	Datatomask		= 0xF0000,
	Clkfreq8shift		= 8,		/* SD clock base divider LSBs */
	Clkfreq8mask		= 0xFF00,
	Clkfreqms2shift		= 6,		/* SD clock base divider MSBs */
	Clkfreqms2mask		= 0xC0,
	Clken			= 1<<2,		/* SD clock enable */
	Clkstable		= 1<<1,	
	Clkintlen		= 1<<0,		/* enable internal EMMC clocks */

	/* Cmdtm */
	Indexshift		= 24,
	Suspend			= 1<<22,
	Resume			= 2<<22,
	Abort			= 3<<22,
	Isdata			= 1<<21,
	Ixchken			= 1<<20,
	Crcchken		= 1<<19,
	Respmask		= 3<<16,
	Respnone		= 0<<16,
	Resp136			= 1<<16,
	Resp48			= 2<<16,
	Resp48busy		= 3<<16,
	Multiblock		= 1<<5,
	Host2card		= 0<<4,
	Card2host		= 1<<4,
	Autocmd12		= 1<<2,
	Autocmd23		= 2<<2,
	Blkcnten		= 1<<1,
	Dmaen			= 1<<0,

	/* Interrupt */
	Admaerr		= 1<<25,
	Acmderr		= 1<<24,
	Acurrenterr	= 1<<23,
	Denderr		= 1<<22,
	Dcrcerr		= 1<<21,
	Dtoerr		= 1<<20,
	Cbaderr		= 1<<19,
	Cenderr		= 1<<18,
	Ccrcerr		= 1<<17,
	Ctoerr		= 1<<16,
	Err		= 1<<15,
	Cardintr	= 1<<8,
	Cardremove	= 1<<7,
	Cardinsert	= 1<<6,
	Readrdy		= 1<<5,
	Writerdy	= 1<<4,
	Dmaintr		= 1<<3,
	Blkgap		= 1<<2,
	Datadone	= 1<<1,
	Cmddone		= 1<<0,

	/* Status */
	Bufread		= 1<<11,
	Bufwrite	= 1<<10,
	Readtrans	= 1<<9,
	Writetrans	= 1<<8,
	Datactive	= 1<<2,
	Datinhibit	= 1<<1,
	Cmdinhibit	= 1<<0,
};

int cmdinfo[64] = {
[0]  Ixchken,
[1]  Resp48,
[2]  Resp136,
[3]  Resp48 | Ixchken | Crcchken,
[5]  Resp48,
[6]  Resp48 | Ixchken | Crcchken,
[7]  Resp48busy | Ixchken | Crcchken,
[8]  Resp48 | Ixchken | Crcchken,
[9]  Resp136,
[11] Resp48 | Ixchken | Crcchken,
[12] Resp48busy | Ixchken | Crcchken,
[13] Resp48 | Ixchken | Crcchken,
[16] Resp48,
[17] Resp48 | Isdata | Card2host | Ixchken | Crcchken | Dmaen,
[18] Resp48 | Isdata | Card2host | Multiblock | Blkcnten | Ixchken | Crcchken | Dmaen,
[24] Resp48 | Isdata | Host2card | Ixchken | Crcchken | Dmaen,
[25] Resp48 | Isdata | Host2card | Multiblock | Blkcnten | Ixchken | Crcchken | Dmaen,
[41] Resp48,
[52] Resp48 | Ixchken | Crcchken,
[53] Resp48	| Ixchken | Crcchken | Isdata | Dmaen,
[55] Resp48 | Ixchken | Crcchken,
};

typedef struct Adma Adma;
typedef struct Ctlr Ctlr;
typedef struct Unit Unit;

/*
 * ADMA2 descriptor
 *	See SD Host Controller Simplified Specification Version 2.00
 */

struct Adma {
	u32int	desc;
	u32int	addr;
};

enum {
	/* desc fields */
	Valid		= 1<<0,
	End			= 1<<1,
	Int			= 1<<2,
	Nop			= 0<<4,
	Tran		= 2<<4,
	Link		= 3<<4,
	OLength		= 16,
	/* maximum value for Length field */
	Maxdma		= ((1<<16) - 4),
};

struct Ctlr {
	Intrcommon;
	QLock	dmalock;
	QLock	butler;
	Rendez	r;
	ulong	extclk;
	Adma	*dma;
	int	numselects;
	Unit	*unit[Maxunit];
};

struct Unit {
	int	index;
	QLock	cmdlock;
	Rendez	r;
	Rendez	cardr;
	int	cardintr;
	u32int	*regs;
	void	*arg;
	int	datadone;
	int	blksize;
};

static Ctlr ctlr;

static void mmcinterrupt(Ureg*, void*);

static uint
clkdiv(uint d)
{
	uint v;

	assert(d < 1<<10);
	v = (d << Clkfreq8shift) & Clkfreq8mask;
	v |= ((d >> 8) << Clkfreqms2shift) & Clkfreqms2mask;
	return v;
}

static int
datadone(void *a)
{
	return ((Unit*)a)->datadone;
}

static Adma*
dmaalloc(void *addr, int len)
{
	int n;
	uintptr a;
	Adma *adma, *p;

	a = (uintptr)addr;
	n = HOWMANY(len, Maxdma);
	adma = sdmalloc(n * sizeof(Adma));
	for(p = adma; len > 0; p++){
		p->desc = Valid | Tran;
		if(n == 1)
			p->desc |= len<<OLength | End | Int;
		else
			p->desc |= Maxdma<<OLength;
		p->addr = PADDR((void*)a);
		a += Maxdma;
		len -= Maxdma;
		n--;
	}
	coherence();
	//allcache->wbse(adma, (char*)p - (char*)adma);
	return adma;
}

static int
sdioinit(void)
{
	u32int *r;
	ulong clk;
	char *s;
	int i, ndev;

	qlock(&ctlr.butler);
	if(ctlr.extclk){
		qunlock(&ctlr.butler);
		return Maxdev;
	}
	r = (u32int*)soc.sdmmc;
	if (probeulong((ulong *)soc.sdmmc, 0) < 0) {
		print("sdiopf: no controller at %#p\n", soc.sdmmc);
		qunlock(&ctlr.butler);
		return 0;
	}
	ndev = Maxdev;
	USED(ndev);
	if(Debug){
		print("sdiopf: regs va %#p pa %#p\n", r, mmuphysaddr((uintptr)r));
		print("sdiopf: Geninfo %ux Capability %ux\n", r[Geninfo],
			r[Capability]);
	}
	/*
	 * can't probe sdiosel; it's always present.
	 */
	if (sbigetimplid() == 1 && sbigetimplvers() <= 6) { /* orig. icicle? */
		ndev = 1;
		soc.sdiosel = 0;
		print("sdiopf: mmc access only, no sd access; need newer hss\n");
	} else if (Debug && soc.sdiosel) {
		print("sdiosel reg: va %#p pa %#p\n", soc.sdiosel,
			mmuphysaddr((uintptr)soc.sdiosel));
		print("sdiosel: %ux\n", *(u32int*)soc.sdiosel);
	}
	if((r[Geninfo] & S0avail) == 0){
		print("sdiopf: no slots!\n");
		qunlock(&ctlr.butler);
		return 0;
	}
	clk = (r[Capability]>>8) & 0xFF;
	clk *= 1000000;
	s = "";
	if(clk == 0){
		s = "Assuming ";
		clk = Extfreq;
	}
	ctlr.extclk = clk;
	print("%ssdio external clock %lud Mhz\n", s, clk/1000000);
	WR(Geninfo, Reset);
	for(i = 1000000; i && (r[Geninfo] & Reset); i--)
		;
	if(i == 0)
		print("sdiopf at %#p didn't reset\n", r);
	WR(Irpten, 0);
	WR(AXIErr, 0);
	WR(Interrupt, ~0);
	qunlock(&ctlr.butler);
	return ndev;
}

static int
sdioinquiry(int subno, char *inquiry, int inqlen)
{
	if(subno >= Maxdev || ctlr.unit[0] == nil)
		return -1;
	return snprint(inquiry, inqlen,
		"SDIO Host Controller: %s",
		subno == 0 ? "SDCard" : "eMMC");
}

static void
unitenable(Unit *unit)
{
	u32int *r;
	int i;

	r = unit->regs;
	WR(Control0, 0);
	delay(1);
	WR(Control0, V3_3 | Buspower | Dwidth1 | DmaADMA2);
	WR(Control1, 0);
	delay(1);
	WR(Control1, clkdiv(ctlr.extclk/Initfreq) |
		DTO<<Datatoshift | Clkintlen);
	for(i = 0; i < 1000; i++){
		delay(1);
		if(r[Control1] & Clkstable)
			break;
	}
	if(i == 1000)
		print("mmc%d: clock won't initialise!\n", unit->index);
	WR(Control1, r[Control1] | Clken);
	WR(Irpten, 0);
	WR(Irptmask, ~0);
	WR(Interrupt, ~0);
}

static void
sdioenable(void)
{
	Unit *unit;
	u32int *r;
	int subno;

	qlock(&ctlr.butler);
	if(ctlr.unit[0]){
		qunlock(&ctlr.butler);
		return;
	}
	r = (u32int*)soc.sdmmc;
	for(subno = 0; subno < Maxunit; subno++){
		unit = (Unit*)malloc(sizeof(Unit));
		unit->index = subno;
		unit->regs = r;
		ctlr.unit[subno] = unit;
		unitenable(unit);
	}
	ctlr.irq = ioconf("sdmmc", 0)->irq;
	enableintr(&ctlr, mmcinterrupt, &ctlr, "sdiopf");
	qunlock(&ctlr.butler);
}

static int
cardintready(void *a)
{
	Unit *unit;

	unit = a;
	return unit->cardintr;
}

static int
sdiocardintr(int subno, int wait)
{
	Unit *unit;
	u32int *r;

	USED(subno);
	unit = ctlr.unit[0];
	assert(unit != nil);
	r = unit->regs;
	r[Irpten] |= Cardintr;
	if(!unit->cardintr){
		if(!wait){
			r[Irpten] &= ~Cardintr;
			return 0;
		}
		sleep(&unit->cardr, cardintready, unit);
	}
	unit->cardintr = 0;
	return 1;
}

static int
awaitdone(Unit *unit, int bits, int ms)
{
	u32int *r;
	int i;

	r = unit->regs;
	WR(Irpten, (r[Irpten] & Cardintr) | bits | Err);
	tsleep(&unit->r, datadone, unit, ms);
	WR(Irpten, r[Irpten] & Cardintr);
	i = unit->datadone;
	unit->datadone = 0;
	return i;
}

static void
uninhibit(u32int *r, int reset, int inhibit)
{
	WR(Control1, r[Control1] | reset);
	while(r[Control1] & reset)
		;
	while(r[Status] & inhibit)
		;
}

static int
sdiocmd(int subno, u32int cmd, u32int arg, u32int *resp)
{
	Unit *unit;
	u32int *r;
	u32int c;
	int i;
	ulong now;

	unit = ctlr.unit[0];
	assert(unit != nil);
	r = unit->regs;
	if (soc.sdiosel)
		*(u32int*)soc.sdiosel = selunit[subno];
	assert(cmd < nelem(cmdinfo) && cmdinfo[cmd] != 0);
	c = (cmd << Indexshift) | cmdinfo[cmd];
	if(cmd == IORWextended){
		if(arg & (1<<31))
			c |= Host2card;
		else
			c |= Card2host;
	}
	if(cmd == SendExtendedCSD && arg == 0)
		c |= Isdata | Card2host | Dmaen;
	if(cmd == Setbuswidth && (arg&0xFFFF0000) != 0)
		c = (c & ~Respmask) | Resp48busy;
	qlock(&ctlr.butler);
	qlock(&unit->cmdlock);
	if(waserror()){
		qunlock(&unit->cmdlock);
		qunlock(&ctlr.butler);
		nexterror();
	}
	if(r[Status] & Cmdinhibit){
		print("mmc%d: need to reset Cmdinhibit intr %ux stat %ux\n",
			subno, r[Interrupt], r[Status]);
		uninhibit(r, Srstcmd, Cmdinhibit);
	}
	if((r[Status] & Datinhibit) &&
	   ((c & Isdata) || (c & Respmask) == Resp48busy)){
		print("mmc%d: need to reset Datinhibit intr %ux stat %ux\n",
			subno, r[Interrupt], r[Status]);
		uninhibit(r, Srstdata, Datinhibit);
	}
	WR(Arg1, arg);
	if((i = (r[Interrupt] & ~Cardintr)) != 0){
		if(i != Cardinsert)
			print("mmc%d: before command, intr was %ux\n", subno, i);
		WR(Interrupt, i);
	}
	WR(Cmdtm, c);
	now = perfticks(); //sys->ticks;
	while(((i=r[Interrupt])&Cmddone) == 0){
		if(perfticks()-now > m->cpuhz) //if(sys->ticks-now > HZ)
			break;
	}
	if((i&(Cmddone|Err)) != Cmddone){
		if((i&~(Cmddone|Err)) != Ctoerr)
			print("mmc%d: cmd %ux error intr %ux stat %ux\n", subno, c, i, r[Status]);
		WR(Interrupt, i);
		if(r[Status]&Cmdinhibit)
			uninhibit(r, Srstcmd, Cmdinhibit);
		if((r[Status] & Datinhibit) && (c & Isdata))
			uninhibit(r, Srstdata, Datinhibit);
		error(Eio);
	}
	WR(Interrupt, i & ~(Dmaintr|Datadone|Readrdy|Writerdy));
	switch(c & Respmask){
	case Resp136:
		resp[0] = r[Resp0]<<8;
		resp[1] = r[Resp0]>>24 | r[Resp1]<<8;
		resp[2] = r[Resp1]>>24 | r[Resp2]<<8;
		resp[3] = r[Resp2]>>24 | r[Resp3]<<8;
		break;
	case Resp48:
	case Resp48busy:
		resp[0] = r[Resp0];
		break;
	case Respnone:
		resp[0] = 0;
		break;
	}
	if((c & Respmask) == Resp48busy){
		i = awaitdone(unit, Datadone, 3000);
		if((i & Datadone) == 0)
			print("mmc%d: no Datadone after CMD%d\n", subno, cmd);
		if(i & Err)
			print("mmc%d: CMD%d error interrupt %ux\n",
				subno, cmd, r[Interrupt]);
		WR(Interrupt, i);
	}
	/*
	 * Once both units are selected, use faster clock (temporary hack)
	 */
	if(cmd == MMCSelect && ++ctlr.numselects > 1){
		delay(10);
		WR(Control1, r[Control1] & ~(Clken|Clkintlen));
		delay(1);
		WR(Control1, clkdiv(ctlr.extclk/SDfreq) |
			DTO<<Datatoshift | Clkintlen);
		for(i = 0; i < 1000; i++){
			delay(1);
			if(r[Control1] & Clkstable)
				break;
		}
		WR(Control1, r[Control1] | Clken);
	}
	/*
	 * If card bus width changes, change host bus width
	 */
	if(cmd == Setbuswidth){
		switch(arg){
		case 0:
		case 0x03B70000:
			WR(Control0, r[Control0] & ~Dwidth4);
			break;
		case 2:
		case 0x03B70100:
			WR(Control0, r[Control0] | Dwidth4);
			break;
		}
	}else if(cmd == IORWdirect && (arg & ~0xFF) == (1<<31|0<<28|7<<9)){
		switch(arg & 0x3){
		case 0:
			WR(Control0, r[Control0] & ~Dwidth4);
			break;
		case 2:
			WR(Control0, r[Control0] | Dwidth4);
			WR(Control0, r[Control0] | Hispeed);
			break;
		}
	}else if(cmd == Voltageswitch){
		WR(Control0, r[Control0] & ~Buspower);
		delay(1);
		WR(Control0, r[Control0] | Buspower | V1_8);
		delay(1);
		WR(Control2, r[Control2] | Sig1_8);
		delay(1);
	}else if(cmd == 0)
		WR(Control0, r[Control0] & ~Dwidth4);
	qunlock(&unit->cmdlock);
	qunlock(&ctlr.butler);
	poperror();
	return 0;
}

void
sdiosetup(int subno, int write, void *buf, int bsize, int bcount)
{
	Unit *unit;
	u32int *r;
	int len;
	USED(write, subno);

	unit = ctlr.unit[0];
	assert(unit != nil);
	qlock(&ctlr.dmalock);
	r = unit->regs;
	if (soc.sdiosel)
		*(u32int*)soc.sdiosel = selunit[subno];
	len = bsize * bcount;
	assert(((uintptr)buf&3) == 0);
	assert((len&3) == 0);
	unit->blksize = bsize;
	WR(Blksizecnt, bcount<<16 | bsize);
	if(ctlr.dma)
		sdfree(ctlr.dma);
	ctlr.dma = dmaalloc(buf, len);
	//if(write)
	//	allcache->wbse(buf, len);
	//else
	//	allcache->wbinvse(buf, len);
	WR(Dmadesc, PADDR(ctlr.dma));
}

static void
sdioio(int subno, int write, uchar *buf, int len)
{
	Unit *unit;
	u32int *r;
	int i;

	USED(buf);
	unit = ctlr.unit[0];
	assert(unit != nil);
	if(len == 0){
		qunlock(&ctlr.dmalock);
		return;
	}
	qlock(&ctlr.butler);
	if(waserror()){
		qunlock(&ctlr.dmalock);
		qunlock(&ctlr.butler);
		nexterror();
	}
	r = unit->regs;
	i = awaitdone(unit, Dmaintr, 3000);
	if((i & (Datadone|Err)) == 0){
		i &= Dmaintr;
		i |= awaitdone(unit, Datadone, 100);
	}
	if((i & (Dmaintr|Datadone|Err)) != (Dmaintr|Datadone)){
		if(i != Dmaintr && i != (Dcrcerr|Err|Dmaintr))
		print("mmc%d: %s error intr %ux stat %ux\n",
			subno, write? "write" : "read", i, r[Status]);
		if(r[Status] & Datinhibit)
			uninhibit(r, Srstdata, Datinhibit);
		WR(Interrupt, i);
		error(Eio);
	}
	qunlock(&ctlr.dmalock);
	qunlock(&ctlr.butler);
	poperror();
}

static void
mmcinterrupt(Ureg*, void*)
{
	Unit *unit;
	u32int *r;
	int subno, i;

	for(subno = 0; subno < Maxunit; subno++){
		unit = ctlr.unit[subno];
		if(unit == nil)
			continue;
		r = unit->regs;
		i = r[Interrupt];
		if(i == 0)
			continue;
		r[Interrupt] = i & (Dmaintr|Datadone|Err);
		if(i & Cardintr){
			i &= ~Cardintr;
			unit->cardintr = 1;
			r[Irpten] &= ~Cardintr;
			wakeup(&unit->cardr);
		}
		if(i){
			unit->datadone |= i;
			wakeup(&unit->r);
		}
	}
}

SDio sdio = {
	"sdpf",
	sdioinit,
	sdioenable,
	sdioinquiry,
	sdiocmd,
	sdiosetup,
	sdioio,
	sdiocardintr,
};
