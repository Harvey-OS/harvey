#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "msaturn.h"

enum{
	UartAoffs = Saturn + 0x0a00,
	UartBoffs = Saturn + 0x0b00,
	Nuart = 2,

	Baudfreq = 14745600 / 16,
	Lcr_div = RBIT(1, uchar),
	Lcr_peven = RBIT(3, uchar),
	Lcr_pen = RBIT(4, uchar),
	Lcr_stop = RBIT(5, uchar),
	Lcr_wrdlenmask = RBIT(6, uchar) | RBIT(7, uchar),
	Lcr_wrdlenshift = 0,
	Lsr_tbre = RBIT(2, uchar),	
	Fcr_txreset = RBIT(5, uchar),
	Fcr_rxreset = RBIT(6, uchar),
	Iir_txempty = RBIT(5, uchar),
	Iir_rxfull = RBIT(6, uchar),
	Iir_rxerr = RBIT(7, uchar),
	Ier_rxerr = RBIT(5, uchar),
	Ier_txempty = RBIT(6, uchar),
	Ier_rxfull = RBIT(7, uchar),
	Lsr_rxavail = RBIT(7, uchar),
	Txsize = 16,
	Rxsize = 16,
};

typedef struct Saturnuart Saturnuart;
struct Saturnuart {
	uchar	rxb;
#define txb	rxb
#define dll	rxb
	uchar	ier;			// Interrupt enable, divisor latch
#define dlm	ier
	uchar	iir;			// Interrupt identification, fifo control
#define fcr	iir	
	uchar	lcr;			// Line control register
	uchar	f1;		
	uchar	lsr;			// Line status register
	ushort	f2;
};

typedef struct UartData UartData;
struct UartData {
	int			suno;	/* saturn uart number: 0 or 1 */
	Saturnuart	*su;
	char			*rxbuf;
	char			*txbuf;
	int			initialized;
	int			enabled;
} uartdata[Nuart];

extern PhysUart saturnphysuart;

Uart suart[Nuart] = {
	{
		.name = "SaturnUart1",
		.baud = 19200,
		.bits = 8,
		.stop = 1,
		.parity = 'n',
		.phys = &saturnphysuart,
		.special = 0,
	},
	{
		.name = "SaturnUart2",
		.baud = 115200,
		.bits = 8,
		.stop = 1,
		.parity = 'n',
		.phys = &saturnphysuart,
		.special = 0,
	},
};

static void suinterrupt(Ureg*, void*);

static Uart*
supnp(void)
{
	int i;

	for (i = 0; i < nelem(suart)-1; i++)
		suart[i].next = &suart[i + 1];
	suart[nelem(suart)-1].next=nil;
	return suart;
}

static void
suinit(Uart*uart)
{
	UartData *ud;
	Saturnuart *su;

	ud = uart->regs;
	su = ud->su;
	su->fcr=Fcr_txreset|Fcr_rxreset;
	ud->initialized=1;
}

static void
suenable(Uart*uart, int ie)
{
	Saturnuart *su;
	UartData *ud;
	int nr;

	nr = uart - suart;
	if (nr < 0 || nr > Nuart)
		panic("No uart %d", nr);
	ud = uartdata + nr;
	ud->suno = nr;
	su=ud->su = (Saturnuart*)((nr == 0)? UartAoffs: UartBoffs);
	uart->regs = ud;

	if(ud->initialized==0)
		suinit(uart);

	if(!ud->enabled && ie){
		intrenable(Vecuart0+nr , suinterrupt, uart, uart->name);
		su->ier=Ier_txempty|Ier_rxfull;
		ud->enabled=1;
	}
}


static long
sustatus(Uart* uart, void* buf, long n, long offset)
{
	Saturnuart *su;
	char p[128];

	su = ((UartData*)uart->regs)->su;
	snprint(p, sizeof p, "b%d c%d e%d l%d m0 p%c s%d i1\n"
		"dev(%d) type(%d) framing(%d) overruns(%d)\n",

		uart->baud,
		uart->hup_dcd, 
		uart->hup_dsr,
		Txsize,
		(su->lcr & Lcr_pen)? ((su->lcr & Lcr_peven) ? 'e': 'o'): 'n',
		(su->lcr & Lcr_stop)? 2: 1,

		uart->dev,
		uart->type,
		uart->ferr,
		uart->oerr);
	n = readstr(offset, buf, n, p);
	free(p);

	return n;
}

static void
sufifo(Uart*, int)
{}

static void
sudtr(Uart*, int)
{}

static void
surts(Uart*, int)
{}

static void
sumodemctl(Uart*, int)
{}

static int
suparity(Uart*uart, int parity)
{
	int lcr;
	Saturnuart *su;

	su = ((UartData*)uart->regs)->su;

	lcr = su->lcr & ~(Lcr_pen|Lcr_peven);

	switch(parity){
	case 'e':
		lcr |= (Lcr_pen|Lcr_peven);
		break;
	case 'o':
		lcr |= Lcr_pen;
		break;
	case 'n':
	default:
		break;
	}

	su->lcr = lcr;
	uart->parity = parity;

	return 0;
}

static int
sustop(Uart* uart, int stop)
{
	int lcr;
	Saturnuart *su;

	su = ((UartData*)uart->regs)->su;
	lcr = su->lcr & ~Lcr_stop;

	switch(stop){
	case 1:
		break;
	case 2:
		lcr |= Lcr_stop;
		break;
	default:
		return -1;
	}

	/* Set new value and reenable if device was previously enabled */
	su->lcr = lcr;
	uart->stop = stop;

	return 0;
}

static int
subits(Uart*uart, int n)
{	
	Saturnuart *su;
	uchar lcr;

	su = ((UartData*)uart->regs)->su;
	if(n<5||n>8)
		return -1;

	lcr = su->lcr & ~Lcr_wrdlenmask;
	lcr |= (n-5) << Lcr_wrdlenshift;
	su->lcr = lcr;
	return 0;
}

static int
subaud(Uart* uart, int baud)
{
	ushort v;
	Saturnuart *su;

	if (uart->enabled){
		su = ((UartData*)uart->regs)->su;
	
		if(baud <= 0)
			return -1;

		v = Baudfreq / baud;
		su->lcr |= Lcr_div;
		su->dll = v;
		su->dlm = v >> 8;
		su->lcr &= ~Lcr_div;
	}
	uart->baud = baud;

	return 0;
}

static void
subreak(Uart*, int)
{}

static void
sukick(Uart *uart)
{
	Saturnuart *su;
	int i;

	if(uart->blocked)
		return;

	su = ((UartData*)uart->regs)->su;
	if((su->iir & Iir_txempty) == 0)
		return;

	for(i = 0; i < Txsize; i++){
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		su->txb = *(uart->op++);
		su->ier |= Ier_txempty;
		break;
	}
}

static void
suputc(Uart *uart, int c)
{
	Saturnuart *su;

	su = ((UartData*)uart->regs)->su;
	while((su->lsr&Lsr_tbre) == 0)
		;

	su->txb=c;
	while((su->lsr&Lsr_tbre) == 0)
			;
}

static int
getchars(Uart *uart, uchar *cbuf)
{
	int nc;
	UartData *ud;
	Saturnuart *su;

	ud = uart->regs;
	su = ud->su;

	while((su->lsr&Lsr_rxavail) == 0)
		;

	*cbuf++ = su->rxb;
	nc = 1;
	while(su->lsr&Lsr_rxavail){
		*cbuf++ = su->rxb;
		nc++;
	}
	return nc;
}

static int
sugetc(Uart *uart)
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
	return c;
}

static void
suinterrupt(Ureg*, void*u)
{
	Saturnuart *su;
	Uart *uart;
	uchar iir;

	uart = u;
	if (uart == nil)
		panic("uart is nil");
	su = ((UartData*)uart->regs)->su;
	iir = su->iir;
	if(iir&Iir_rxfull)
		while(su->lsr&Lsr_rxavail)
			uartrecv(uart, su->rxb);
	if(iir & Iir_txempty){
		su->ier&=~Ier_txempty;
		uartkick(uart);
	}
	if (iir & Iir_rxerr)
		uart->oerr++;
	intack();
}

static void
sudisable(Uart* uart)
{
	Saturnuart *su;

	su = ((UartData*)uart->regs)->su;
	su->ier&=~(Ier_txempty|Ier_rxfull);
}

PhysUart saturnphysuart = {
	.name		= "su",
	.pnp			= supnp,
	.enable		= suenable,
	.disable		= sudisable,
	.kick			= sukick,
	.dobreak		= subreak,
	.baud		= subaud,
	.bits			= subits,
	.stop			= sustop,
	.parity		= suparity,
	.modemctl	= sumodemctl,
	.rts			= surts,
	.dtr			= sudtr,
	.status		= sustatus,
	.fifo			= sufifo,
	.getc			= sugetc,
	.putc			= suputc,
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
	if(n < 0 || n >= nelem(suart))
		return;

	uart = suart + n;

/*	uartctl(uart, "b115200 l8 pn s1"); */
	if(*cmd != '\0')
		uartctl(uart, cmd);
	(*uart->phys->enable)(uart, 0);

	consuart = uart;
	uart->console = 1;
} 

Saturnuart*uart = (Saturnuart*)UartAoffs;

void
dbgputc(int c)
{
	while((uart->lsr&Lsr_tbre) == 0)
		;

	uart->txb=c;
	while((uart->lsr&Lsr_tbre) == 0)
			;
}

void
dbgputs(char*s)
{
	while(*s)
		dbgputc(*s++);
}

void
dbgputx(ulong x)
{
	int i;
	char c;

	for(i=0; i < sizeof(ulong) * 2; i++){
		c = ((x >> (28 - i * 4))) & 0xf;
		if(c >= 0 && c <= 9)
			c += '0';
		else
			c += 'a' - 10;

		while((uart->lsr&Lsr_tbre) == 0)
			;

		uart->txb=c;
	}
	while((uart->lsr&Lsr_tbre) == 0)
			;

	uart->txb='\n';
	while((uart->lsr&Lsr_tbre) == 0)
			;
}
