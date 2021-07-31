#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "imm.h"
#include "../port/error.h"
#include "../ppc/uartsmc.h"

/*
 * PowerPC 8260 SMC UART
 */

enum {
	/* SMC Mode Registers */
	Clen		= 0x7800,	/* Character length */
	Sl		= 0x0400,	/* Stop length, 0: one stop bit, 1: two */
	Pen		= 0x0200,	/* Parity enable */
	Pm		= 0x0100,	/* Parity mode, 0 is odd */
	Sm		= 0x0030,	/* SMC mode, two bits */
	SMUart	= 0x0020,	/* SMC mode, 0b10 is uart */
	Dm		= 0x000c,	/* Diagnostic mode, 00 is normal */
	Ten		= 0x0002,	/* Transmit enable, 1 is enabled */
	Ren		= 0x0001,	/* Receive enable, 1 is enabled */

	/* SMC Event/Mask Registers */
	ce_Brke	= 0x0040,	/* Break end */
	ce_Br	= 0x0020,	/* Break character received */
	ce_Bsy	= 0x0004,	/* Busy condition */
	ce_Txb	= 0x0002,	/* Tx buffer */
	ce_Rxb	= 0x0001,	/* Rx buffer */

	/* Receive/Transmit Buffer Descriptor Control bits */
	BDContin=	1<<9,
	BDIdle=		1<<8,
	BDPreamble=	1<<8,
	BDBreak=	1<<5,
	BDFrame=	1<<4,
	BDParity=	1<<3,
	BDOverrun=	1<<1,

	/* Tx and Rx buffer sizes (32 bytes) */
	Rxsize=		CACHELINESZ,
	Txsize=		CACHELINESZ,
};

extern PhysUart smcphysuart;

Uart smcuart[Nuart] = {
	{
		.name = "SMC1",
		.baud = 115200,
		.bits = 8,
		.stop = 1,
		.parity = 'n',
		.phys = &smcphysuart,
		.special = 0,
	},
/*	Only configure SMC1 for now
	{
		.name = "SMC2",
		.baud = 115200,
		.bits = 8,
		.stop = 1,
		.parity = 'n',
		.phys = &smcphysuart,
		.special = 0,
	},
*/
};

int uartinited = 0;

static void smcinterrupt(Ureg*, void*);
static void smcputc(Uart *uart, int c);

int
baudgen(int baud)
{
	int d;

	d = ((m->brghz+(baud>>1))/baud)>>4;
	if(d >= (1<<12))
		return ((d+15)>>3)|1;
	return d<<1;
}

static Uart*
smcpnp(void)
{
	int i;

	for (i = 0; i < nelem(smcuart) - 1; i++)
		smcuart[i].next = smcuart + i + 1;
	return smcuart;
}

void
smcinit(Uart *uart)
{
	Uartsmc *p;
	SMC *smc;
	UartData *ud;
	ulong lcr;
	int bits;

	ud = uart->regs;

	if (ud->initialized)
		return;

	smcsetup(uart);	/* Steps 1 through 4, PPC-dependent */
	p = ud->usmc;
	smc = ud->smc;

	/* step 5: set up buffer descriptors */
	/* setup my uart structure */
	if (ud->rxb == nil)
		ud->rxb = bdalloc(1);
	if (ud->txb == nil)
		ud->txb = bdalloc(1);

	p->rbase = ((ulong)ud->rxb) - (ulong)IMMR;
	p->tbase = ((ulong)ud->txb) - (ulong)IMMR;

	/* step 8: receive buffer size */
	p->mrblr = Rxsize;

	/* step 9: */
	p->maxidl = 15;

	/* step 10: */
	p->brkln = 0;
	p->brkec = 0;

	/* step 11: */
	p->brkcr = 0;

	/* step 12: setup receive buffer */
	ud->rxb->status = BDEmpty|BDWrap|BDInt;
	ud->rxb->length = 0;
	ud->rxbuf = xspanalloc(Rxsize, 0, CACHELINESZ);
	ud->rxb->addr = PADDR(ud->rxbuf);

	/* step 13: step transmit buffer */
	ud->txb->status = BDWrap|BDInt;
	ud->txb->length = 0;
	ud->txbuf = xspanalloc(Txsize, 0, CACHELINESZ);
	ud->txb->addr = PADDR(ud->txbuf);

	/* step 14: clear events */
	smc->smce = ce_Brke | ce_Br | ce_Bsy | ce_Txb | ce_Rxb;

	/* 
	 * step 15: enable interrupts (done later)
	 * smc->smcm = ce_Brke | ce_Br | ce_Bsy | ce_Txb | ce_Rxb;
	 */

	/* step 17: set parity, no of bits, UART mode, ... */
	lcr = SMUart;
	bits = uart->bits + 1;

	switch(uart->parity){
	case 'e':
		lcr |= (Pen|Pm);
		bits +=1;
		break;
	case 'o':
		lcr |= Pen;
		bits +=1;
		break;
	case 'n':
	default:
		break;
	}

	if(uart->stop == 2){
		lcr |= Sl;
		bits += 1;
	}

	/* Set new value and reenable if device was previously enabled */
	smc->smcmr = lcr |  bits <<11 | 0x3;

	ud->initialized = 1;
}

static void
smcenable(Uart *uart, int intenb)
{
	UartData *ud;
	SMC *smc;
	int nr;

	nr = uart - smcuart;
	if (nr < 0 || nr > Nuart)
		panic("No SMC %d", nr);
	ud = uartdata + nr;
	ud->smcno = nr;
	uart->regs = ud;
	if (ud->initialized == 0)
		smcinit(uart);
	if (ud->enabled || intenb == 0)
		return;
	smc = ud->smc;
	/* clear events */
	smc->smce = ce_Brke | ce_Br | ce_Bsy | ce_Txb | ce_Rxb;
	/* enable interrupts */
	smc->smcm = ce_Brke | ce_Br | ce_Bsy | ce_Txb | ce_Rxb;
	intrenable(VecSMC1 + ud->smcno, smcinterrupt, uart, uart->name);
	ud->enabled = 1;
}

static long
smcstatus(Uart* uart, void* buf, long n, long offset)
{
	SMC *sp;
	char p[128];

	sp = ((UartData*)uart->regs)->smc;
	snprint(p, sizeof p, "b%d c%d e%d l%d m0 p%c s%d i1\n"
		"dev(%d) type(%d) framing(%d) overruns(%d)\n",

		uart->baud,
		uart->hup_dcd, 
		uart->hup_dsr,
		((sp->smcmr & Clen) >>11) - ((sp->smcmr&Pen) ? 1 : 0) - ((sp->smcmr&Sl) ? 2 : 1),
		(sp->smcmr & Pen) ? ((sp->smcmr & Pm) ? 'e': 'o'): 'n',
		(sp->smcmr & Sl) ? 2: 1,

		uart->dev,
		uart->type,
		uart->ferr,
		uart->oerr 
	);
	n = readstr(offset, buf, n, p);
	free(p);

	return n;
}

static void
smcfifo(Uart*, int)
{
	/*
	 * Toggle FIFOs:
	 * if none, do nothing;
	 * reset the Rx and Tx FIFOs;
	 * empty the Rx buffer and clear any interrupt conditions;
	 * if enabling, try to turn them on.
	 */
	return;
}

static void
smcdtr(Uart*, int)
{
}

static void
smcrts(Uart*, int)
{
}

static void
smcmodemctl(Uart*, int)
{
}

static int
smcparity(Uart* uart, int parity)
{
	int lcr;
	SMC *sp;

	sp = ((UartData*)uart->regs)->smc;

	lcr = sp->smcmr & ~(Pen|Pm);

	/* Disable transmitter/receiver. */
	sp->smcmr &= ~(Ren | Ten);

	switch(parity){
	case 'e':
		lcr |= (Pen|Pm);
		break;
	case 'o':
		lcr |= Pen;
		break;
	case 'n':
	default:
		break;
	}
	/* Set new value and reenable if device was previously enabled */
	sp->smcmr = lcr;

	uart->parity = parity;

	return 0;
}

static int
smcstop(Uart* uart, int stop)
{
	int lcr, bits;
	SMC *sp;

	sp = ((UartData*)uart->regs)->smc;
	lcr = sp->smcmr & ~(Sl | Clen);

	/* Disable transmitter/receiver. */
	sp->smcmr &= ~(Ren | Ten);

	switch(stop){
	case 1:
		break;
	case 2:
		lcr |= Sl;
		break;
	default:
		return -1;
	}

	bits = uart->bits + ((lcr & Pen) ? 1 : 0) + ((lcr & Sl) ? 2 : 1);
	lcr |= bits<<11;

	/* Set new value and reenable if device was previously enabled */
	sp->smcmr = lcr;

	uart->stop = stop;

	return 0;
}

static int
smcbits(Uart* uart, int bits)
{
	int lcr, b;
	SMC *sp;

	if (bits < 5 || bits > 14)
		return -1;

	sp = ((UartData*)uart->regs)->smc;
	lcr = sp->smcmr & ~Clen;

	b = bits + ((sp->smcmr & Pen) ? 1 : 0) + ((sp->smcmr & Sl) ? 2 : 1);

	if (b > 15)
		return -1;

	/* Disable transmitter/receiver */
	sp->smcmr &= ~(Ren | Ten);

	/* Set new value and reenable if device was previously enabled */
	sp->smcmr = lcr |  b<<11;

	uart->bits = bits;

	return 0;
}

static int
smcbaud(Uart* uart, int baud)
{
	int i;
	SMC *sp;

	if (uart->enabled){
		sp = ((UartData*)uart->regs)->smc;
	
		if(uart->freq == 0 || baud <= 0)
			return -1;
	
		i = sp - imm->smc;
		imm->brgc[i] = (((m->brghz >> 4) / baud) << 1) | 0x00010000;
	}
	uart->baud = baud;

	return 0;
}

static void
smcbreak(Uart*, int)
{
}

static void
smckick(Uart *uart)
{
	BD *txb;
	UartData *ud;
	int i;

	if(uart->blocked)
		return;

	ud = uart->regs;
	txb = ud->txb;

	if (txb->status & BDReady)
		return;	/* Still busy */

	for(i = 0; i < Txsize; i++){
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		ud->txbuf[i] = *(uart->op++);
	}
	if (i == 0)
		return;
	dcflush(ud->txbuf, Txsize);
	txb->length = i;
	sync();
	txb->status |= BDReady|BDInt;
}

static void
smcinterrupt(Ureg*, void* u)
{
	int i, nc;
	char *buf;
	BD *rxb;
	UartData *ud;
	Uart *uart;
	uchar events;

	uart = u;
	if (uart == nil)
		panic("uart is nil");
	ud = uart->regs;
	if (ud == nil)
		panic("ud is nil");

	events = ud->smc->smce;
	ud->smc->smce = events;	/* Clear events */

	if (events & 0x10)
		iprint("smc%d: break\n", ud->smcno);
	if (events & 0x4)
		uart->oerr++;
	if (events & 0x1){
		/* Receive characters
		*/
		rxb = ud->rxb;
		buf = ud->rxbuf;
		dczap(buf, Rxsize);	/* invalidate data cache before copying */
		if ((rxb->status & BDEmpty) == 0){
			nc = rxb->length;
			for (i=0; i<nc; i++)
				uartrecv(uart, *buf++);
			sync();
			rxb->status |= BDEmpty;
		}else{
			iprint("uartsmc: unexpected receive event\n");
		}
	}
	if (events & 0x2){
		if ((ud->txb->status & BDReady) == 0)
			uartkick(uart);
	}
}

static void
smcdisable(Uart* uart)
{
	SMC *sp;

	sp = ((UartData*)uart->regs)->smc;
	sp->smcmr &= ~(Ren | Ten);
}

static int
getchars(Uart *uart, uchar *cbuf)
{
	int i, nc;
	char *buf;
	BD *rxb;
	UartData *ud;

	ud = uart->regs;
	rxb = ud->rxb;

	/* Wait for character to show up.
	*/
	buf = ud->rxbuf;
	while (rxb->status & BDEmpty)
		;
	nc = rxb->length;
	for (i=0; i<nc; i++)
		*cbuf++ = *buf++;
	sync();
	rxb->status |= BDEmpty;

	return(nc);
}

static int
smcgetc(Uart *uart)
{
	static uchar buf[128], *p;
	static int cnt;
	char	c;

	if (cnt <= 0) {
		cnt = getchars(uart, buf);
		p = buf;
	}
	c = *p++;
	cnt--;

	return(c);
}

static void
smcputc(Uart *uart, int c)
{
	BD *txb;
	UartData *ud;
	SMC *smc;

	ud = uart->regs;
	txb = ud->txb;
	smc = ud->smc;
	smc->smcm = 0;

	/* Wait for last character to go.
	*/
	while (txb->status & BDReady)
		;

	ud->txbuf[0] = c;
	dcflush(ud->txbuf, 1);
	txb->length = 1;
	sync();
	txb->status |= BDReady;

	while (txb->status & BDReady)
		;
}

PhysUart smcphysuart = {
	.name		= "smc",
	.pnp			= smcpnp,
	.enable		= smcenable,
	.disable		= smcdisable,
	.kick			= smckick,
	.dobreak		= smcbreak,
	.baud		= smcbaud,
	.bits			= smcbits,
	.stop			= smcstop,
	.parity		= smcparity,
	.modemctl	= smcmodemctl,
	.rts			= smcrts,
	.dtr			= smcdtr,
	.status		= smcstatus,
	.fifo			= smcfifo,
	.getc			= smcgetc,
	.putc			= smcputc,
};

void
console(void)
{
	Uart *uart;
	int n;
	char *cmd, *p;

	if((p = getconf("console")) == nil)
		return;
	n = strtoul(p, &cmd, 0);
	if(p == cmd)
		return;
	if(n < 0 || n >= nelem(smcuart))
		return;
	uart = smcuart + n;

/*	uartctl(uart, "b115200 l8 pn s1"); */
	if(*cmd != '\0')
		uartctl(uart, cmd);
	(*uart->phys->enable)(uart, 0);

	consuart = uart;
	uart->console = 1;
} 
