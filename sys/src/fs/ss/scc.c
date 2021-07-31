#include "all.h"
#include "io.h"
#include "mem.h"

/*
 *  Driver for the Z8530.
 */
enum
{
	/* wr 0 */
	ResExtPend=	2<<3,
	ResTxPend=	5<<3,
	ResErr=		6<<3,

	/* wr 1 */
	ExtIntEna=	1<<0,
	TxIntEna=	1<<1,
	RxIntDis=	0<<3,
	RxIntFirstEna=	1<<3,
	RxIntAllEna=	2<<3,

	/* wr 3 */
	RxEna=		1,
	Rx5bits=	0<<6,
	Rx7bits=	1<<6,
	Rx6bits=	2<<6,
	Rx8bits=	3<<6,

	/* wr 4 */
	SyncMode=	0<<2,
	Rx1stop=	1<<2,
	Rx1hstop=	2<<2,
	Rx2stop=	3<<2,
	X16=		1<<6,

	/* wr 5 */
	TxRTS=		1<<1,
	TxEna=		1<<3,
	TxBreak=	1<<4,
	TxDTR=		1<<7,
	Tx5bits=	0<<5,
	Tx7bits=	1<<5,
	Tx6bits=	2<<5,
	Tx8bits=	3<<5,

	/* wr 9 */
	IntEna=		1<<3,
	ResetB=		1<<6,
	ResetA=		2<<6,
	HardReset=	3<<6,

	/* wr 11 */
	TRxCOutBR=	2,
	TxClockBR=	2<<3,
	RxClockBR=	2<<5,
	TRxCOI=		1<<2,

	/* wr 14 */
	BREna=		1,
	BRSource=	2,

	/* rr 0 */
	RxReady=	1,
	TxReady=	1<<2,
	RxDCD=		1<<3,
	RxCTS=		1<<5,
	RxBreak=	1<<7,

	/* rr 3 */
	ExtPendB=	1,	
	TxPendB=	1<<1,
	RxPendB=	1<<2,
	ExtPendA=	1<<3,	
	TxPendA=	1<<4,
	RxPendA=	1<<5,
};

typedef struct
{
	ushort	sticky[16];	/* sticky write register values */
	uchar	*ptr;		/* command/pointer register in Z8530 */
	uchar	*data;		/* data register in Z8530 */
	int	printing;	/* true if printing */
	ulong	freq;		/* clock frequency */

	void	(*rx)(int);	/* routine to take a received character */
	int	(*tx)(void);	/* routine to get a character to transmit */
}SCC;

static SCC scc[2];

static void
onepointseven(void)
{
	int i;

	for(i = 0; i < 20; i++)
		;
}

static void
sccwrreg(SCC *sp, int addr, int value)
{
	onepointseven();
	*sp->ptr = addr;
	onepointseven();
	*sp->ptr = sp->sticky[addr] | value;
}

static ushort
sccrdreg(SCC *sp, int addr)
{
	onepointseven();
	*sp->ptr = addr;
	onepointseven();
	return *sp->ptr;
}

static void
sccsetbaud(SCC *sp, int rate)
{
	int brconst;

	brconst = (sp->freq+16*rate-1)/(2*16*rate) - 2;

	sccwrreg(sp, 12, brconst & 0xff);
	sccwrreg(sp, 13, (brconst>>8) & 0xff);
}

static void
sccdtr(SCC *sp, int n)
{
	if(n)
		sp->sticky[5] |= TxDTR;
	else
		sp->sticky[5] &=~TxDTR;
	sccwrreg(sp, 5, 0);
}

static void
sccrts(SCC *sp, int n)
{
	if(n)
		sp->sticky[5] |= TxRTS;
	else
		sp->sticky[5] &=~TxRTS;
	sccwrreg(sp, 5, 0);
}

static void
sccsetup0(SCC *sp)
{
	memset(sp->sticky, 0, sizeof(sp->sticky));

	/*
	 *  turn on baud rate generator and set rate to 9600 baud.
	 *  use 1 stop bit.
	 */
	sp->sticky[14] = BRSource;
	sccwrreg(sp, 14, 0);
	sccsetbaud(sp, 9600);
	sp->sticky[4] = Rx1stop | X16;
	sccwrreg(sp, 4, 0);
	sp->sticky[11] = TxClockBR | RxClockBR | TRxCOutBR | TRxCOI;
	sccwrreg(sp, 11, 0);
	sp->sticky[14] = BREna | BRSource;
	sccwrreg(sp, 14, 0);

	/*
	 *  enable I/O, 8 bits/character
	 */
	sp->sticky[3] = RxEna | Rx8bits;
	sccwrreg(sp, 3, 0);
	sp->sticky[5] = TxEna | Tx8bits;
	sccwrreg(sp, 5, 0);
}

void
sccsetup(void *addr, ulong freq)
{
	SCCdev *dev;
	SCC *sp;

	dev = addr;

	sp = scc;
	sp->ptr = &dev->ptra;
	sp->data = &dev->dataa;
	sp->freq = freq;
	sccsetup0(sp);
	sp++;
	sp->ptr = &dev->ptrb;
	sp->data = &dev->datab;
	sp->freq = freq;
	sccsetup0(sp);
}

static void
sccenable(SCC *sp)
{
	/*
 	 *  turn on interrupts
	 */
	if(sp->tx)
		sp->sticky[1] |= TxIntEna | ExtIntEna;
	if(sp->rx)
		sp->sticky[1] |= RxIntAllEna | ExtIntEna;
	sccwrreg(sp, 1, 0);
	sp->sticky[9] |= IntEna;
	sccwrreg(sp, 9, 0);

	/*
	 *  turn on DTR and RTS
	 */
	sccdtr(sp, 1);
	sccrts(sp, 1);
}

void
sccspecial(int port, void (*rx)(int), int (*tx)(void), int baud)
{
	SCC *sp = &scc[port];

	sp->rx = rx;
	sp->tx = tx;
	sccenable(sp);
	sccsetbaud(sp, baud);
}

static void
sccintr0(SCC *sp, uchar x)
{
	int c;

	if(x & ExtPendB)
		sccwrreg(sp, 0, ResExtPend);
	if(x & RxPendB){
		while(*sp->ptr&RxReady){
			onepointseven();
			c = *sp->data;
if(c == 4) firmware();
			(*sp->rx)(c & 0xFF);
		}
	}
	if(x & TxPendB){
		c = (*sp->tx)();
		onepointseven();
		if(c == -1)
			sccwrreg(sp, 0, ResTxPend);
		else
			*sp->data = c;
	}
}

void
sccintr(void)
{
	uchar x;

	x = sccrdreg(scc, 3);
	if(x & (ExtPendB|RxPendB|TxPendB))
		sccintr0(&scc[1], x);
	x = x >> 3;
	if(x & (ExtPendB|RxPendB|TxPendB))
		sccintr0(&scc[0], x);
}

int
sccgetc(int port)
{
	SCC *sp = &scc[port];

	if(*sp->ptr&RxReady)
		return *sp->data & 0xFF;
	return 0;
}

void
sccputc(int port, int c)
{
	SCC *sp = &scc[port];
	int i;

	for(i=0; i<100; i++) {
		if(*sp->ptr&TxReady)
			break;
		delay(1);
	}
	*sp->data = c;
}
