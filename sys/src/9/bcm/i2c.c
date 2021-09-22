/*
 * bcm2835 i2c controller
 *
 *	Only i2c1 is supported.
 *	i2c2 is reserved for HDMI.
 *	i2c0 SDA0/SCL0 pins are not routed to P1 connector (except for early Rev 0 boards)
 *
 * maybe hardware problems lurking, see: https://github.com/raspberrypi/linux/issues/254
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"../port/error.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#define I2CREGS	(VIRTIO+0x804000)
#define SDA0Pin	2
#define	SCL0Pin	3
#define	Alt0	0x4

typedef struct I2c I2c;
typedef struct Bsc Bsc;

/*
 * Registers for Broadcom Serial Controller (i2c compatible)
 */
struct Bsc {
	u32int	ctrl;
	u32int	stat;
	u32int	dlen;
	u32int	addr;
	u32int	fifo;
	u32int	clkdiv;		/* default 1500 => 100 KHz assuming 150Mhz input clock */
	u32int	delay;		/* default (48<<16)|48 falling:rising edge */
	u32int	clktimeout;	/* default 64 */
};

/*
 * Per-controller info
 */
struct I2c {
	QLock	lock;
	Lock	reglock;
	Rendez	r;
	Bsc	*regs;
};

static I2c i2c;

enum {
	/* ctrl */
	I2cen	= 1<<15,	/* I2c enable */
	Intr	= 1<<10,	/* interrupt on reception */
	Intt	= 1<<9,		/* interrupt on transmission */
	Intd	= 1<<8,		/* interrupt on done */
	Start	= 1<<7,		/* aka ST, start a transfer */
	Clear	= 1<<4,		/* clear fifo */
//	Read	= 1<<0,		/* read transfer */
//	Write	= 0<<0,		/* write transfer */

	/* stat */
	Clkt	= 1<<9,		/* clock stretch timeout */
	Err	= 1<<8,			/* NAK */
	Rxf	= 1<<7,			/* RX fifo full */
	Txe	= 1<<6,			/* TX fifo full */
	Rxd	= 1<<5,			/* RX fifo has data */
	Txd	= 1<<4,			/* TX fifo has space */
	Rxr	= 1<<3,			/* RX fiio needs reading */
	Txw	= 1<<2,			/* TX fifo needs writing */
	Done	= 1<<1,		/* transfer done */
	Ta	= 1<<0,			/* Transfer active */

	/* maximum I2C I/O (can change) */
	MaxIO =	128,
	MaxSA =	2,	/* longest subaddress */
	Bufsize = (MaxIO+MaxSA+1+4)&~3,		/* extra space for subaddress/clock bytes and alignment */

	Chatty = 0,
};

static void
i2cinterrupt(Ureg*, void*)
{
	Bsc *r;
	int st;

	r = i2c.regs;
	st = 0;
	if((r->ctrl & Intr) && (r->stat & Rxd))
		st |= Intr;
	if((r->ctrl & Intt) && (r->stat & Txd))
		st |= Intt;
	if(r->stat & Done)
		st |= Intd;
	if(st){
		r->ctrl &= ~st;
		wakeup(&i2c.r);
	}
}

static int
i2cready(void *st)
{
	return (i2c.regs->stat & (uintptr)st);
}

static void
i2cinit(void)
{
	i2c.regs = (Bsc*)I2CREGS;
	i2c.regs->clkdiv = 2500;

	gpiosel(SDA0Pin, Alt0);
	gpiosel(SCL0Pin, Alt0);
	gpiopullup(SDA0Pin);
	gpiopullup(SCL0Pin);

	intrenable(IRQi2c, i2cinterrupt, 0, 0, "i2c");
}

/*
 * To do subaddressing avoiding a STOP condition between the address and payload.
 * 	- write the subaddress,
 *	- poll until the transfer starts,
 *	- overwrite the registers for the payload transfer, before the subaddress
 * 		transaction has completed.
 *
 * FIXME: neither 10bit adressing nor 100Khz transfers implemented yet.
 */
static void
i2cio(int rw, int tenbit, uint addr, void *buf, int len, int salen, uint subaddr)
{
	Bsc *r;
	uchar *p;
	int st;

	if(tenbit)
		error("10bit addressing not supported");
	if(salen == 0 && subaddr)	/* default subaddress len == 1byte */
		salen = 1;

	qlock(&i2c.lock);
	r = i2c.regs;
	r->ctrl = I2cen | Clear;
	r->addr = addr;
	r->stat = Clkt|Err|Done;

	if(salen){
		r->dlen = salen;
		r->ctrl = I2cen | Start | Write;
		while((r->stat & Ta) == 0) {
			if(r->stat & (Err|Clkt)) {
				qunlock(&i2c.lock);
				error(Eio);
			}
		}
		r->dlen = len;
		r->ctrl = I2cen | Start | Intd | rw;
		for(; salen > 0; salen--)
			r->fifo = subaddr >> ((salen-1)*8);
		/*
		 * Adapted from Linux code...uses undocumented
		 * status information.
		 */
		if(rw == Read) {
			do {
				if(r->stat & (Err|Clkt)) {
					qunlock(&i2c.lock);
					error(Eio);
				}
				st = r->stat >> 28;
			} while(st != 0 && st != 4 && st != 5);
		}
	}
	else {
		r->dlen = len;
		r->ctrl = I2cen | Start | Intd | rw;
	}

	p = buf;
	st = rw == Read? Rxd : Txd;
	while(len > 0){
		while((r->stat & (st|Done)) == 0){
			r->ctrl |= rw == Read? Intr : Intt;
			sleep(&i2c.r, i2cready, (void*)(st|Done));
		}
		if(r->stat & (Err|Clkt)){
			qunlock(&i2c.lock);
			error(Eio);
		}
		if(rw == Read){
			do{
				*p++ = r->fifo;
				len--;
			}while ((r->stat & Rxd) && len > 0);
		}else{
			do{
				r->fifo = *p++;
				len--;
			}while((r->stat & Txd) && len > 0);
		}
	}
	while((r->stat & Done) == 0)
		sleep(&i2c.r, i2cready, (void*)Done);
	if(r->stat & (Err|Clkt)){
		qunlock(&i2c.lock);
		error(Eio);
	}
	r->ctrl = 0;
	qunlock(&i2c.lock);
}


void
i2csetup(int)
{
	//print("i2csetup\n");
	i2cinit();
}

long
i2csend(I2Cdev *d, void *buf, long len, ulong offset)
{
	i2cio(Write, d->tenbit, d->addr, buf, len, d->salen, offset);
	return len;
}

long
i2crecv(I2Cdev *d, void *buf, long len, ulong offset)
{
	i2cio(Read, d->tenbit, d->addr, buf, len, d->salen, offset);
	return len;
}
