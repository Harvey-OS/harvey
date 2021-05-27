/*
 * bcm2835 sdhost controller
 *
 * Copyright © 2016 Richard Miller <r.miller@acm.org>
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/sd.h"

extern SDio *sdcardlink;

#define SDHOSTREGS	(VIRTIO+0x202000)

enum {
	Extfreq		= 250*Mhz,	/* guess external clock frequency if vcore doesn't say */
	Initfreq	= 400000,	/* initialisation frequency for MMC */
	SDfreq		= 25*Mhz,	/* standard SD frequency */
	SDfreqhs	= 50*Mhz,	/* SD high speed frequency */
	FifoDepth	= 4,		/* "Limit fifo usage due to silicon bug" (linux driver) */

	GoIdle		= 0,		/* mmc/sdio go idle state */
	MMCSelect	= 7,		/* mmc/sd card select command */
	Setbuswidth	= 6,		/* mmc/sd set bus width command */
	Switchfunc	= 6,		/* mmc/sd switch function command */
	Stoptransmission = 12,	/* mmc/sd stop transmission command */
	Appcmd = 55,			/* mmc/sd application command prefix */
};

enum {
	/* Controller registers */
	Cmd		= 0x00>>2,
	Arg		= 0x04>>2,
	Timeout		= 0x08>>2,
	Clkdiv		= 0x0c>>2,

	Resp0		= 0x10>>2,
	Resp1		= 0x14>>2,
	Resp2		= 0x18>>2,
	Resp3		= 0x1c>>2,

	Status		= 0x20>>2,
	Poweron		= 0x30>>2,
	Dbgmode		= 0x34>>2,
	Hconfig		= 0x38>>2,
	Blksize		= 0x3c>>2,
	Data		= 0x40>>2,
	Blkcount	= 0x50>>2,

	/* Cmd */
	Start		= 1<<15,
	Failed		= 1<<14,
	Respmask	= 7<<9,
	Resp48busy	= 4<<9,
	Respnone	= 2<<9,
	Resp136		= 1<<9,
	Resp48		= 0<<9,
	Host2card	= 1<<7,
	Card2host	= 1<<6,

	/* Status */
	Busyint		= 1<<10,
	Blkint		= 1<<9,
	Sdioint		= 1<<8,
	Rewtimeout	= 1<<7,
	Cmdtimeout	= 1<<6,
	Crcerror	= 3<<4,
	Fifoerror	= 1<<3,
	Dataflag	= 1<<0,
	Intstatus	= (Busyint|Blkint|Sdioint|Dataflag),
	Errstatus	= (Rewtimeout|Cmdtimeout|Crcerror|Fifoerror),

	/* Hconfig */
	BusyintEn	= 1<<10,
	BlkintEn	= 1<<8,
	SdiointEn	= 1<<5,
	DataintEn	= 1<<4,
	Slowcard	= 1<<3,
	Extbus4		= 1<<2,
	Intbuswide	= 1<<1,
	Cmdrelease	= 1<<0,
};

static int cmdinfo[64] = {
[0]  Start | Respnone,
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
	Rendez	r;
	int	bcount;
	int	done;
	ulong	extclk;
	int	appcmd;
};

static Ctlr sdhost;

static void sdhostinterrupt(Ureg*, void*);

static void
WR(int reg, u32int val)
{
	u32int *r = (u32int*)SDHOSTREGS;

	if(0)print("WR %2.2ux %ux\n", reg<<2, val);
	r[reg] = val;
}

static int
datadone(void*)
{
	return sdhost.done;
}

static void
sdhostclock(uint freq)
{
	uint div;

	div = sdhost.extclk / freq;
	if(sdhost.extclk / freq > freq)
		div++;
	if(div < 2)
		div = 2;
	WR(Clkdiv, div - 2);
}

static int
sdhostinit(void)
{
	u32int *r;
	ulong clk;
	int i;

	/* disconnect emmc and connect sdhost to SD card gpio pins */
	for(i = 48; i <= 53; i++)
		gpiosel(i, Alt0);
	clk = getclkrate(ClkCore);
	if(clk == 0){
		clk = Extfreq;
		print("sdhost: assuming external clock %lud Mhz\n", clk/1000000);
	}
	sdhost.extclk = clk;
	sdhostclock(Initfreq);
	r = (u32int*)SDHOSTREGS;
	WR(Poweron, 0);
	WR(Timeout, 0xF00000);
	WR(Dbgmode, FINS(r[Dbgmode], 9, 10, (FifoDepth | FifoDepth<<5)));
	return 0;
}

static int
sdhostinquiry(char *inquiry, int inqlen)
{
	return snprint(inquiry, inqlen, "BCM SDHost Controller");
}

static void
sdhostenable(void)
{
	u32int *r;

	r = (u32int*)SDHOSTREGS;
	USED(r);
	WR(Poweron, 1);
	delay(10);
	WR(Hconfig, Intbuswide | Slowcard | BusyintEn);
	WR(Clkdiv, 0x7FF);
	intrenable(IRQsdhost, sdhostinterrupt, nil, 0, "sdhost");
}

static int
sdhostcmd(u32int cmd, u32int arg, u32int *resp)
{
	u32int *r;
	u32int c;
	int i;
	ulong now;

	r = (u32int*)SDHOSTREGS;
	assert(cmd < nelem(cmdinfo) && cmdinfo[cmd] != 0);
	c = cmd | cmdinfo[cmd];
	/*
	 * CMD6 may be Setbuswidth or Switchfunc depending on Appcmd prefix
	 */
	if(cmd == Switchfunc && !sdhost.appcmd)
		c |= Card2host;
	if(cmd != Stoptransmission && (i = (r[Dbgmode] & 0xF)) > 2){
		print("sdhost: previous command stuck: Dbg=%d Cmd=%ux\n", i, r[Cmd]);
		error(Eio);
	}
	/*
	 * GoIdle indicates new card insertion: reset bus width & speed
	 */
	if(cmd == GoIdle){
		WR(Hconfig, r[Hconfig] & ~Extbus4);
		sdhostclock(Initfreq);
	}

	if(r[Status] & (Errstatus|Dataflag))
		WR(Status, Errstatus|Dataflag);
	sdhost.done = 0;
	WR(Arg, arg);
	WR(Cmd, c);
	now = m->ticks;
	while(((i=r[Cmd])&(Start|Failed)) == Start)
		if(m->ticks-now > HZ)
			break;
	if((i&(Start|Failed)) != 0){
		if(r[Status] != Cmdtimeout)
			print("sdhost: cmd %ux arg %ux error stat %ux\n", i, arg, r[Status]);
		i = r[Status];
		WR(Status, i);
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
	case Resp48busy:
		resp[0] = r[Resp0];
		break;
	case Respnone:
		resp[0] = 0;
		break;
	}
	if((c & Respmask) == Resp48busy){
		tsleep(&sdhost.r, datadone, 0, 3000);
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
				WR(Hconfig, r[Hconfig] & ~Extbus4);
				break;
			case 2:
				WR(Hconfig, r[Hconfig] | Extbus4);
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
sdhostiosetup(int write, void *buf, int bsize, int bcount)
{
	USED(write);
	USED(buf);

	sdhost.bcount = bcount;
	WR(Blksize, bsize);
	WR(Blkcount, bcount);
}

static void
sdhostio(int write, uchar *buf, int len)
{
	u32int *r;
	int piolen;
	u32int w;

	r = (u32int*)SDHOSTREGS;
	assert((len&3) == 0);
	assert((PTR2UINT(buf)&3) == 0);
	okay(1);
	if(waserror()){
		okay(0);
		nexterror();
	}
	/*
	 * According to comments in the linux driver, the hardware "doesn't
	 * manage the FIFO DREQs properly for multi-block transfers" on input,
	 * so the dma must be stopped early and the last 3 words fetched with pio
	 */
	piolen = 0;
	if(!write && sdhost.bcount > 1){
		piolen = (FifoDepth-1) * sizeof(u32int);
		len -= piolen;
	}
	if(write)
		dmastart(DmaChanSdhost, DmaDevSdhost, DmaM2D,
			buf, &r[Data], len);
	else
		dmastart(DmaChanSdhost, DmaDevSdhost, DmaD2M,
			&r[Data], buf, len);
	if(dmawait(DmaChanSdhost) < 0)
		error(Eio);
	if(!write){
		cachedinvse(buf, len);
		buf += len;
		for(; piolen > 0; piolen -= sizeof(u32int)){
			if((r[Dbgmode] & 0x1F00) == 0){
				print("sdhost: FIFO empty after short dma read\n");
				error(Eio);
			}
			w = r[Data];
			*((u32int*)buf) = w;
			buf += sizeof(u32int);
		}
	}
	poperror();
	okay(0);
}

static void
sdhostinterrupt(Ureg*, void*)
{	
	u32int *r;
	int i;

	r = (u32int*)SDHOSTREGS;
	i = r[Status];
	WR(Status, i);
	if(i & Busyint){
		sdhost.done = 1;
		wakeup(&sdhost.r);
	}
}

SDio sdiohost = {
	"sdhost",
	sdhostinit,
	sdhostenable,
	sdhostinquiry,
	sdhostcmd,
	sdhostiosetup,
	sdhostio,
};

void
sdhostlink(void)
{
	sdcardlink = &sdiohost;
}
