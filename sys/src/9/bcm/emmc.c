/*
 * bcm2835 external mass media controller (mmc / sd host interface)
 *
 * Copyright © 2012 Richard Miller <r.miller@acm.org>
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/sd.h"

#define EMMCREGS	(VIRTIO+0x300000)

enum {
	Extfreq		= 100*Mhz,	/* guess external clock frequency if */
					/* not available from vcore */
	Initfreq	= 400000,	/* initialisation frequency for MMC */
	SDfreq		= 25*Mhz,	/* standard SD frequency */
	DTO		= 14,		/* data timeout exponent (guesswork) */

	MMCSelect	= 7,		/* mmc/sd card select command */
	Setbuswidth	= 6,		/* mmc/sd set bus width command */
};

enum {
	/* Controller registers */
	Arg2			= 0x00>>2,
	Blksizecnt		= 0x04>>2,
	Arg1			= 0x08>>2,
	Cmdtm			= 0x0c>>2,
	Resp0			= 0x10>>2,
	Resp1			= 0x14>>2,
	Resp2			= 0x18>>2,
	Resp3			= 0x1c>>2,
	Data			= 0x20>>2,
	Status			= 0x24>>2,
	Control0		= 0x28>>2,
	Control1		= 0x2c>>2,
	Interrupt		= 0x30>>2,
	Irptmask		= 0x34>>2,
	Irpten			= 0x38>>2,
	Control2		= 0x3c>>2,
	Forceirpt		= 0x50>>2,
	Boottimeout		= 0x70>>2,
	Dbgsel			= 0x74>>2,
	Exrdfifocfg		= 0x80>>2,
	Exrdfifoen		= 0x84>>2,
	Tunestep		= 0x88>>2,
	Tunestepsstd		= 0x8c>>2,
	Tunestepsddr		= 0x90>>2,
	Spiintspt		= 0xf0>>2,
	Slotisrver		= 0xfc>>2,

	/* Control0 */
	Dwidth4			= 1<<1,
	Dwidth1			= 0<<1,

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
	Clkgendiv		= 0<<5,		/* SD clock divided */
	Clkgenprog		= 1<<5,		/* SD clock programmable */
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

	/* Interrupt */
	Acmderr		= 1<<24,
	Denderr		= 1<<22,
	Dcrcerr		= 1<<21,
	Dtoerr		= 1<<20,
	Cbaderr		= 1<<19,
	Cenderr		= 1<<18,
	Ccrcerr		= 1<<17,
	Ctoerr		= 1<<16,
	Err		= 1<<15,
	Cardintr	= 1<<8,		/* not in Broadcom datasheet */
	Cardinsert	= 1<<6,		/* not in Broadcom datasheet */
	Readrdy		= 1<<5,
	Writerdy	= 1<<4,
	Datadone	= 1<<1,
	Cmddone		= 1<<0,

	/* Status */
	Bufread		= 1<<11,	/* not in Broadcom datasheet */
	Bufwrite	= 1<<10,	/* not in Broadcom datasheet */
	Readtrans	= 1<<9,
	Writetrans	= 1<<8,
	Datactive	= 1<<2,
	Datinhibit	= 1<<1,
	Cmdinhibit	= 1<<0,
};

int cmdinfo[64] = {
[0]  Ixchken,
[2]  Resp136,
[3]  Resp48 | Ixchken | Crcchken,
[6]  Resp48 | Ixchken | Crcchken,
[7]  Resp48busy | Ixchken | Crcchken,
[8]  Resp48 | Ixchken | Crcchken,
[9]  Resp136,
[12] Resp48busy | Ixchken | Crcchken,
[13] Resp48 | Ixchken | Crcchken,
[16] Resp48,
[17] Resp48 | Isdata | Card2host | Ixchken | Crcchken,
[18] Resp48 | Isdata | Card2host | Multiblock | Blkcnten | Ixchken | Crcchken,
[24] Resp48 | Isdata | Host2card | Ixchken | Crcchken,
[25] Resp48 | Isdata | Host2card | Multiblock | Blkcnten | Ixchken | Crcchken,
[41] Resp48,
[55] Resp48 | Ixchken | Crcchken,
};

typedef struct Ctlr Ctlr;

struct Ctlr {
	Rendez	r;
	int	datadone;
	int	fastclock;
	ulong	extclk;
};

static Ctlr emmc;

static void mmcinterrupt(Ureg*, void*);

static void
WR(int reg, u32int val)
{
	u32int *r = (u32int*)EMMCREGS;

	if(0)print("WR %2.2ux %ux\n", reg<<2, val);
	microdelay(emmc.fastclock? 2: 20);
	r[reg] = val;
}

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
datadone(void*)
{
	return emmc.datadone;
}

static int
emmcinit(void)
{
	u32int *r;
	ulong clk;
	char *s;

	clk = getclkrate(ClkEmmc);
	s = "";
	if(clk == 0){
		s = "Assuming ";
		clk = Extfreq;
	}
	emmc.extclk = clk;
	print("%seMMC external clock %lud Mhz\n", s, clk/1000000);
	r = (u32int*)EMMCREGS;
	if(0)print("emmc control %8.8ux %8.8ux %8.8ux\n",
		r[Control0], r[Control1], r[Control2]);
	WR(Control1, Srsthc);
	delay(10);
	while(r[Control1] & Srsthc)
		;
	return 0;
}

static int
emmcinquiry(char *inquiry, int inqlen)
{
	u32int *r;
	uint ver;

	r = (u32int*)EMMCREGS;
	ver = r[Slotisrver] >> 16;
	return snprint(inquiry, inqlen,
		"Arasan eMMC SD Host Controller %2.2x Version %2.2x",
		ver&0xFF, ver>>8);
}

static void
emmcenable(void)
{
	u32int *r;
	int i;

	r = (u32int*)EMMCREGS;
	WR(Control1, clkdiv(emmc.extclk / Initfreq - 1) | DTO << Datatoshift |
		Clkgendiv | Clken | Clkintlen);
	for(i = 0; i < 1000; i++){
		delay(1);
		if(r[Control1] & Clkstable)
			break;
	}
	if(i == 1000)
		print("SD clock won't initialise!\n");
	WR(Irptmask, ~(Dtoerr|Cardintr));
	intrenable(IRQmmc, mmcinterrupt, nil, 0, "mmc");
}

static int
emmccmd(u32int cmd, u32int arg, u32int *resp)
{
	u32int *r;
	u32int c;
	int i;
	ulong now;

	r = (u32int*)EMMCREGS;
	assert(cmd < nelem(cmdinfo) && cmdinfo[cmd] != 0);
	c = (cmd << Indexshift) | cmdinfo[cmd];
	if(r[Status] & Cmdinhibit){
		print("emmccmd: need to reset Cmdinhibit intr %ux stat %ux\n",
			r[Interrupt], r[Status]);
		WR(Control1, r[Control1] | Srstcmd);
		while(r[Control1] & Srstcmd)
			;
		while(r[Status] & Cmdinhibit)
			;
	}
	if((c & Isdata || (c & Respmask) == Resp48busy) &&
	    r[Status] & Datinhibit){
		print("emmccmd: need to reset Datinhibit intr %ux stat %ux\n",
			r[Interrupt], r[Status]);
		WR(Control1, r[Control1] | Srstdata);
		while(r[Control1] & Srstdata)
			;
		while(r[Status] & Datinhibit)
			;
	}
	WR(Arg1, arg);
	if((i = r[Interrupt]) != 0){
		if(i != Cardinsert)
			print("emmc: before command, intr was %ux\n", i);
		WR(Interrupt, i);
	}
	WR(Cmdtm, c);
	now = m->ticks;
	while(((i=r[Interrupt])&(Cmddone|Err)) == 0)
		if(m->ticks-now > HZ)
			break;
	if((i&(Cmddone|Err)) != Cmddone){
		if((i&~Err) != Ctoerr)
			print("emmc: cmd %ux error intr %ux stat %ux\n", c, i, r[Status]);
		WR(Interrupt, i);
		if(r[Status]&Cmdinhibit){
			WR(Control1, r[Control1]|Srstcmd);
			while(r[Control1]&Srstcmd)
				;
		}
		error(Eio);
	}
	WR(Interrupt, i & ~(Datadone|Readrdy|Writerdy));
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
		WR(Irpten, Datadone|Err);
		tsleep(&emmc.r, datadone, 0, 3000);
		i = emmc.datadone;
		emmc.datadone = 0;
		WR(Irpten, 0);
		if((i & Datadone) == 0)
			print("emmcio: no Datadone after CMD%d\n", cmd);
		if(i & Err)
			print("emmcio: CMD%d error interrupt %ux\n",
				cmd, r[Interrupt]);
		WR(Interrupt, i);
	}
	/*
	 * Once card is selected, use faster clock
	 */
	if(cmd == MMCSelect){
		delay(10);
		WR(Control1, clkdiv(emmc.extclk / SDfreq - 1) |
			DTO << Datatoshift | Clkgendiv | Clken | Clkintlen);
		for(i = 0; i < 1000; i++){
			delay(1);
			if(r[Control1] & Clkstable)
				break;
		}
		delay(10);
		emmc.fastclock = 1;
	}
	/*
	 * If card bus width changes, change host bus width
	 */
	if(cmd == Setbuswidth)
		switch(arg){
		case 0:
			WR(Control0, r[Control0] & ~Dwidth4);
			break;
		case 2:
			WR(Control0, r[Control0] | Dwidth4);
			break;
		}
	return 0;
}

void
emmciosetup(int write, void *buf, int bsize, int bcount)
{
	USED(write);
	USED(buf);
	WR(Blksizecnt, bcount<<16 | bsize);
}

static void
emmcio(int write, uchar *buf, int len)
{
	u32int *r;
	int i;

	r = (u32int*)EMMCREGS;
	assert((len&3) == 0);
	okay(1);
	if(waserror()){
		okay(0);
		nexterror();
	}
	if(write)
		dmastart(DmaChanEmmc, DmaDevEmmc, DmaM2D,
			buf, &r[Data], len);
	else
		dmastart(DmaChanEmmc, DmaDevEmmc, DmaD2M,
			&r[Data], buf, len);
	if(dmawait(DmaChanEmmc) < 0)
		error(Eio);
	WR(Irpten, Datadone|Err);
	tsleep(&emmc.r, datadone, 0, 3000);
	i = emmc.datadone;
	emmc.datadone = 0;
	WR(Irpten, 0);
	if((i & Datadone) == 0){
		print("emmcio: %d timeout intr %ux stat %ux\n",
			write, i, r[Status]);
		WR(Interrupt, i);
		error(Eio);
	}
	if(i & Err){
		print("emmcio: %d error intr %ux stat %ux\n",
			write, r[Interrupt], r[Status]);
		WR(Interrupt, i);
		error(Eio);
	}
	if(i)
		WR(Interrupt, i);
	poperror();
	okay(0);
}

static void
mmcinterrupt(Ureg*, void*)
{	
	u32int *r;
	int i;

	r = (u32int*)EMMCREGS;
	i = r[Interrupt];
	r[Interrupt] = i & (Datadone|Err);
	emmc.datadone = i;
	wakeup(&emmc.r);
}

SDio sdio = {
	"emmc",
	emmcinit,
	emmcenable,
	emmcinquiry,
	emmccmd,
	emmciosetup,
	emmcio,
};
