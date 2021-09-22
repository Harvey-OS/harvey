/*
 * Synopsis DWC sdio host controller
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/sd.h"

enum {
	Extfreq		= 100000000,	/* BeagleV Starlight external clock frequency */
	Initfreq	= 400000,	/* initialisation frequency for MMC */
	SDfreq		= 25000000,	/* standard SD frequency */
	SDfreqhs	= 50000000,	/* SD high speed frequency */

	GoIdle		= 0,		/* mmc/sdio go idle state */
	MMCSelect	= 7,		/* mmc/sd card select command */
	Setbuswidth	= 6,		/* mmc/sd set bus width command */
	Switchfunc	= 6,		/* mmc/sd switch function command */
	Stoptransmission = 12,	/* mmc/sd stop transmission command */
	Appcmd = 55,			/* mmc/sd application command prefix */
};

enum {
	/* Controller registers */
	Ctrl	= 0x00>>2,
	Poweron	= 0x04>>2,
	Clkdiv	= 0x08>>2,
	Clksrc	= 0x0C>>2,
	Clkena	= 0x10>>2,
	Timeout	= 0x14>>2,
	Ctype	= 0x18>>2,
	Blksize	= 0x1C>>2,
	Bytecnt	= 0x20>>2,
	Intmask	= 0x24>>2,
	Arg	= 0x28>>2,
	Cmd	= 0x2C>>2,
	Resp0	= 0x30>>2,
	Resp1	= 0x34>>2,
	Resp2	= 0x38>>2,
	Resp3	= 0x3C>>2,
	Mintsts	= 0x40>>2,
	Rintsts	= 0x44>>2,
	Status	= 0x48>>2,
	Fifoth	= 0x4C>>2,
	Hcon	= 0x70>>2,
	Uhsreg	= 0x74>>2,
	Busmode	= 0x80>>2,
	Dbaddr	= 0x88>>2,
	Idsts	= 0x8C>>2,
	Idinten	= 0x90>>2,
	Dscaddr	= 0x94>>2,
	Bufaddr	= 0x98>>2,
	Data	= 0x200>>2,

	/* Intmask, Rintsts, Mintsts */
	Resperr	= 1<<1,
	Cdone	= 1<<2,
	Dto	= 1<<3,
	Txdr	= 1<<4,
	Rxdr	= 1<<5,
	Rcrc	= 1<<6,
	Ctimeout= 1<<8,

	/* Idinten, Idsts */
	Errdma	= 1<<9,
	Ndma	= 1<<8,
	Txdma	= 1<<0,
	Rxdma	= 1<<1,

	/* Cmd */
	Start		= 1<<31,
	Holdreg		= 1<<29,
	Updclk		= 1<<21,
	Stop		= 1<<14,
	Prvdatwait	= 1<<13,
	Respmask	= 3<<6,
	Respnone	= 0<<6,
	Resp48		= 1<<6,
	Resp48busy	= 1<<6,
	Resp136		= 3<<6,
	Host2card	= 3<<9,
	Card2host	= 1<<9,

	/* Status */
	Fifoempty	= 1<<2,
	Fifofull	= 1<<3,
	Busy		= 1<<9,

	/* Ctype */
	Extbus1		= 0,
	Extbus4		= 1,
	Extbus8		= 1<<16,

	/* Ctrl */
	Reset		= 1<<0,
	Resetfifo	= 1<<1,
	Resetdma	= 1<<2,
	Intenable	= 1<<4,
	Dmaen		= 1<<5,
	Idmacen		= 1<<25,

	/* Busmode */
	Idreset		= 1<<0,
	Fixedburst	= 1<<1,
	Idmac		= 1<<7,
};

/* internal dma controller descriptors */
typedef struct Adma Adma;
struct Adma {
	u32int	desc;
	u32int	len;
	u32int	addr;
	u32int	next;
};

enum {
	Maxdma	= 8192,

	/* desc flags */
	Own	= 1<<31,
	Ces	= 1<<30,
	Chained	= 1<<4,
	First	= 1<<3,
	Last	= 1<<2,
	Noint	= 1<<1,
};
	
static int cmdinfo[64] = {
[0]  Start | Respnone,
[1]  Start | Resp48,
[2]  Start | Resp136,
[3]  Start | Resp48,
[5]  Start | Resp48,
[6]  Start | Resp48,
[63]  Start | Resp48 | Card2host,
[7]  Start | Resp48busy,
[8]  Start | Resp48,
[9]  Start | Resp136,
[11] Start | Resp48,
[12] Start | Resp48busy,
[13] Start | Resp48,
[16] Start | Resp48,
[17] Start | Resp48 | Card2host,
[18] Start | Resp48 | Card2host,
[24] Start | Resp48 | Host2card,
[25] Start | Resp48 | Host2card,
[41] Start | Resp48,
[52] Start | Resp48,
[53] Start | Resp48,
[55] Start | Resp48,
};

typedef struct Ctlr Ctlr;

struct Ctlr {
	Intrcommon;
	Rendez	r;
	Adma	*adma;
	int	admalen;
	ulong	extclk;
	int	appcmd;
};

static Ctlr sdhost;

static void sdhostinterrupt(Ureg*, void*);

static void
WR(int reg, u32int val)
{
	u32int *r = (u32int*)soc.sdmmc;

	r[reg] = val;
}

static int
datadone(void *arg)
{
	u32int *r;

	r = (u32int*)arg;
	return *r == 0;
}

static Adma*
dmaalloc(void *addr, int len)
{
	int n, desclen, flags;
	uintptr a;
	Adma *adma, *p;

	a = (uintptr)addr;
	n = HOWMANY(len, Maxdma);
	desclen = (n * sizeof(Adma) + CACHELINESZ) & ~CACHELINESZ;
	if(desclen <= sdhost.admalen)
		adma = sdhost.adma;
	else{
		free(sdhost.adma);
		adma = mallocalign((n * sizeof(Adma) + CACHELINESZ) & ~CACHELINESZ,
			CACHELINESZ, 0, 0);
		sdhost.adma = adma;
		sdhost.admalen = desclen;
	}
	flags = Own | First | Noint;
	for(p = adma; len > 0; p++){
		if(n == 1)
			flags = (flags & ~Noint) | Last;
		p->desc = flags;
		flags &= ~First;
		p->addr = PADDR((void*)a);
		if(len <= Maxdma/2){
			p->next = 0;
			p->len = len;
		}else{
			p->next = p->addr + Maxdma/2;
			if(len <= Maxdma)
				p->len = (Maxdma/2) | (len-Maxdma/2) << 13;
			else
				p->len = (Maxdma/2) | (Maxdma/2) << 13;
		}
		a += Maxdma;
		len -= Maxdma;
		n--;
	}
	l2cacheflush(adma, (char*)p - (char*)adma);
	return adma;
}

static void
sdhostclock(uint freq)
{
	uint div;
	u32int *r;

	r = (u32int*)soc.sdmmc;
	div = sdhost.extclk / (2*freq);
	WR(Clkena, 0);
	WR(Clksrc, 0);
	WR(Clkdiv, div);
	WR(Cmd, Updclk | Prvdatwait | Start);
	while(r[Cmd] & Start)
		;
	WR(Clkena, 0x10001);
	WR(Cmd, Updclk | Prvdatwait | Start);
	while(r[Cmd] & Start)
		;
}

static int
sdhostinit(void)
{
	u32int *r;
	ulong clk;

	r = (u32int*)soc.sdmmc;
	clk = Extfreq;
	print("%s: assuming external clock %lud Mhz\n", sdio.name, clk/1000000);
	sdhost.extclk = clk;
	WR(Poweron, 1);
	delay(10);
	WR(Ctrl, Reset|Resetfifo|Resetdma);
	while(r[Ctrl] & (Reset|Resetfifo|Resetdma))
		;
	sdhostclock(Initfreq);
	WR(Timeout, ~0);
	WR(Rintsts, ~0);
	WR(Intmask, 0);
	WR(Idinten, 0);
	WR(Busmode, Idreset);
	WR(Dbaddr, 0);
	return 1;
}

static int
sdhostinquiry(int, char *inquiry, int inqlen)
{
	return snprint(inquiry, inqlen, "DWC SD Host Controller");
}

static void
sdhostenable(void)
{
	u32int *r;

	r = (u32int*)soc.sdmmc;
	USED(r);
	sdhost.irq = ioconf("sdmmc", 0)->irq;
	enableintr(&sdhost, sdhostinterrupt, &sdhost, sdio.name);
	WR(Ctrl, Intenable);
}

static int
sdhostcmd(int, u32int cmd, u32int arg, u32int *resp)
{
	u32int *r;
	u32int c;
	int i;
	ulong now;

	r = (u32int*)soc.sdmmc;
	assert(cmd < nelem(cmdinfo) && cmdinfo[cmd] != 0);
	c = cmd | cmdinfo[cmd];
	/*
	 * CMD6 may be Setbuswidth or Switchfunc depending on Appcmd prefix
	 */
	if(cmd == Switchfunc && !sdhost.appcmd)
		c |= Card2host;
	if(cmd != Stoptransmission)
		c |= Prvdatwait;
	else
		c |= Stop;
	/*
	 * GoIdle indicates new card insertion: reset bus width & speed
	 */
	if(cmd == GoIdle){
		WR(Ctype, Extbus1);
		WR(Uhsreg, 0);
		sdhostclock(Initfreq);
	}

	now = sys->ticks;
	if((c&Stop) == 0){
		while(r[Status] & Busy){
			if(sys->ticks-now > 3*HZ){
				iprint("%s: busy cmd %d stat %ux\n", sdio.name, cmd, r[Status]);
				break;
			}
			sched();
		}
	}
	WR(Rintsts, ~0);
	WR(Arg, arg);
	WR(Cmd, c | Holdreg);
	now = sys->ticks;
	while(((i=r[Rintsts]) & Cdone) == 0)
		if(sys->ticks-now > HZ){
			iprint("%s: timeout cmd %d intr %ux stat %ux\n", sdio.name, cmd, i, r[Status]);
			error(Eio);
		}
	if(i & (Ctimeout|Resperr|Rcrc)){
		WR(Rintsts, i);
		iprint("%s: error cmd %d intr %ux stat %ux\n", sdio.name, cmd, i, r[Status]);
		error(Eio);
	}
	switch(c & Respmask){
	case Resp136:
		resp[0] = r[Resp0];
		resp[1] = r[Resp1];
		resp[2] = r[Resp2];
		resp[3] = r[Resp3];
		break;
	case Resp48:
	/* case Resp48busy: */
		resp[0] = r[Resp0];
		break;
	case Respnone:
		resp[0] = 0;
		break;
	}
	switch(cmd) {
	case MMCSelect:
		/*
		 * Once card is selected, use faster clock
		 */
		delay(1);
		sdhostclock(SDfreq);
		delay(1);
		break;
	case Setbuswidth:
		if(sdhost.appcmd){
			/*
			 * If card bus width changes, change host bus width
			 */
			switch(arg){
			case 0:
				WR(Ctype, Extbus1);
				break;
			case 2:
				WR(Ctype, Extbus4);
				break;
			}
		}else{
			/*
			 * If card switched into high speed mode, increase clock speed
			 */
			if((arg&0x8000000F) == 0x80000001){
				delay(1);
				sdhostclock(SDfreqhs);
				delay(1);
			}
		}
		break;
	}
	sdhost.appcmd = (cmd == Appcmd);
	return 0;
}

void
sdhostiosetup(int, int write, void *buf, int bsize, int bcount)
{
	u32int *r;
	Adma *adma;
	int len;
	USED(write);

	len = bsize * bcount;
	adma = dmaalloc(buf, len);
	l2cacheflush(buf, len);
	r = (u32int*)soc.sdmmc;
	WR(Idsts, ~0);
	WR(Dbaddr, (u32int)PADDR(adma));
	WR(Ctrl, r[Ctrl] | (Dmaen|Idmacen));
	WR(Busmode, Fixedburst | Idmac);
	WR(Blksize, bsize);
	WR(Bytecnt, len);
	WR(Rintsts, Dto);
}

static void
sdhostio(int, int write, uchar *buf, int len)
{
	u32int *r;
	int mask;

	r = (u32int*)soc.sdmmc;
	assert((len&3) == 0);
	assert((PTR2UINT(buf)&3) == 0);
	if(waserror()){
		WR(Idinten, 0);
		nexterror();
	}
	mask = write ? Txdma : Rxdma;
	WR(Idinten, mask|Ndma|Errdma);
	tsleep(&sdhost.r, datadone, &r[Idinten], 3000);
	if(r[Idinten] != 0){
		WR(Idinten, 0);
		iprint("%s: dma timeout idsts %ux intsts %ux status %ux\n", sdio.name,
			r[Idsts], r[Rintsts], r[Status]);
		error(Eio);
	}
	if((r[Rintsts] & Dto) == 0){
		WR(Intmask, Dto);
		tsleep(&sdhost.r, datadone, &r[Intmask], 3000);
		if(r[Intmask] != 0){
			WR(Intmask, 0);
			iprint("%s: data timeout idsts %ux intsts %ux status %ux\n", sdio.name,
				r[Idsts], r[Rintsts], r[Status]);
			error(Eio);
		}
	}
	if((r[Rintsts] & ~(Cdone|Txdr|Rxdr|Dto)) != 0){
			iprint("%s: data err idsts %ux intsts %ux status %ux\n", sdio.name,
				r[Idsts], r[Rintsts], r[Status]);
			error(Eio);
	}
	if(!write)
		l2cacheflush(buf, len);
	poperror();
}

static void
sdhostinterrupt(Ureg*, void*)
{	
	u32int *r;
	int i;

	r = (u32int*)soc.sdmmc;
	i = r[Idsts];
	if(i){
		if(i & Errdma)
			iprint("sddwc idsts %x\n", i);
		WR(Idsts, i);
		WR(Idinten, 0);
		if(i & (Txdma|Rxdma))
			wakeup(&sdhost.r);
	}
	i = r[Mintsts];
	if(i){
		WR(Intmask, 0);
		wakeup(&sdhost.r);
	}
}

SDio sdio = {
	"sddwc",
	sdhostinit,
	sdhostenable,
	sdhostinquiry,
	sdhostcmd,
	sdhostiosetup,
	sdhostio,
};
