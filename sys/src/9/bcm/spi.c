/*
 * bcm2835 spi controller
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"../port/error.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#define SPIREGS	(VIRTIO+0x204000)
#define	SPI0_CE1_N	7		/* P1 pin 26 */
#define SPI0_CE0_N	8		/* P1 pin 24 */
#define SPI0_MISO	9		/* P1 pin 21 */
#define	SPI0_MOSI	10		/* P1 pin 19 */
#define	SPI0_SCLK	11		/* P1 pin 23 */

typedef struct Ctlr Ctlr;
typedef struct Spiregs Spiregs;

/*
 * Registers for main SPI controller
 */
struct Spiregs {
	u32int	cs;		/* control and status */
	u32int	data;
	u32int	clkdiv;
	u32int	dlen;
	u32int	lossitoh;
	u32int	dmactl;
};

/*
 * Per-controller info
 */
struct Ctlr {
	Spiregs	*regs;
	QLock	lock;
	Lock	reglock;
	Rendez	r;
};

static Ctlr spi;

enum {
	/* cs */
	Lossi32bit	= 1<<25,
	Lossidma	= 1<<24,
	Cspol2		= 1<<23,
	Cspol1		= 1<<22,
	Cspol0		= 1<<21,
	Rxf		= 1<<20,
	Rxr		= 1<<19,
	Txd		= 1<<18,
	Rxd		= 1<<17,
	Done		= 1<<16,
	Lossi		= 1<<13,
	Ren		= 1<<12,
	Adcs		= 1<<11,	/* automatically deassert chip select (dma) */
	Intr		= 1<<10,
	Intd		= 1<<9,
	Dmaen		= 1<<8,
	Ta		= 1<<7,
	Cspol		= 1<<6,
	Rxclear		= 1<<5,
	Txclear		= 1<<4,
	Cpol		= 1<<3,
	Cpha		= 1<<2,
	Csmask		= 3<<0,
	Csshift		= 0,

	/* dmactl */
	Rpanicshift	= 24,
	Rdreqshift	= 16,
	Tpanicshift	= 8,
	Tdreqshift	= 0,
};

static void
spiinit(void)
{
	spi.regs = (Spiregs*)SPIREGS;
	spi.regs->clkdiv = 250;		/* 1 MHz */
	gpiosel(SPI0_MISO, Alt0);
	gpiosel(SPI0_MOSI, Alt0);
	gpiosel(SPI0_SCLK, Alt0);
	gpiosel(SPI0_CE0_N,  Alt0);
	gpiosel(SPI0_CE1_N,  Alt0);
}

void
spimode(int mode)
{
	if(spi.regs == 0)
		spiinit();
	spi.regs->cs = (spi.regs->cs & ~(Cpha | Cpol)) | (mode << 2);
}

/*
 * According the Broadcom docs, the divisor has to
 * be a power of 2, but an errata says that should
 * be multiple of 2 and scope observations confirm
 * that restricting it to a power of 2 is unnecessary.
 */
void
spiclock(uint mhz)
{
	if(spi.regs == 0)
		spiinit();
	if(mhz == 0) {
		spi.regs->clkdiv = 32768;	/* about 8 KHz */
		return;
	}
	spi.regs->clkdiv = 2 * ((125 + (mhz - 1)) / mhz);
}

void
spirw(uint cs, void *buf, int len)
{
	Spiregs *r;

	assert(cs <= 2);
	assert(len < (1<<16));
	qlock(&spi.lock);
	if(waserror()){
		qunlock(&spi.lock);
		nexterror();
	}
	if(spi.regs == 0)
		spiinit();
	r = spi.regs;
	r->dlen = len;
	r->cs = (r->cs & (Cpha | Cpol)) | (cs << Csshift) | Rxclear | Txclear | Dmaen | Adcs | Ta;
	/*
	 * Start write channel before read channel - cache wb before inv
	 */
	dmastart(DmaChanSpiTx, DmaDevSpiTx, DmaM2D,
		buf, &r->data, len);
	dmastart(DmaChanSpiRx, DmaDevSpiRx, DmaD2M,
		&r->data, buf, len);
	if(dmawait(DmaChanSpiRx) < 0)
		error(Eio);
	cachedinvse(buf, len);
	r->cs &= (Cpha | Cpol);
	qunlock(&spi.lock);
	poperror();
}
