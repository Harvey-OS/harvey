#include	"all.h"

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

typedef struct SCC	SCC;
struct SCC
{
	ushort	sticky[16];	/* sticky write register values */
	uchar	*ptr;		/* command/pointer register in Z8530 */
	uchar	*data;		/* data register in Z8530 */
	int	printing;	/* true if printing */
	ulong	freq;		/* clock frequency */

	/* console interface */
	IOQ	*iq;		/* input character queue */
	IOQ	*oq;		/* output character queue */
};

int	nscc;
SCC	*scc[8];	/* up to 4 8530's */

void
onepointseven(void)
{
	int i;
	for(i = 0; i < 20; i++)
		;
}

/*
 *  Access registers using the pointer in register 0.
 *  This is a bit stupid when accessing register 0.
 */
void
sccwrreg(SCC *sp, int addr, int value)
{
	onepointseven();
	*sp->ptr = addr;
	wbflush();
	onepointseven();
	*sp->ptr = sp->sticky[addr] | value;
	wbflush();
}
ushort
sccrdreg(SCC *sp, int addr)
{
	onepointseven();
	*sp->ptr = addr;
	wbflush();
	onepointseven();
	return *sp->ptr;
}

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
void
sccsetbaud(SCC *sp, int rate)
{
	int brconst;

	brconst = (sp->freq+16*rate-1)/(2*16*rate) - 2;

	sccwrreg(sp, 12, brconst & 0xff);
	sccwrreg(sp, 13, (brconst>>8) & 0xff);
}

/*
 *  toggle DTR
 */
void
sccdtr(SCC *sp, int n)
{
	if(n)
		sp->sticky[5] |= TxDTR;
	else
		sp->sticky[5] &=~TxDTR;
	sccwrreg(sp, 5, 0);
}

/*
 *  toggle RTS
 */
void
sccrts(SCC *sp, int n)
{
	if(n)
		sp->sticky[5] |= TxRTS;
	else
		sp->sticky[5] &=~TxRTS;
	sccwrreg(sp, 5, 0);
}

/*
 *  default is 9600 baud, 1 stop bit, 8 bit chars, no interrupts,
 *  transmit and receive enabled, interrupts disabled.
 */
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

	/*
	 *  allocate a structure, set port addresses, and setup the line
	 */
	sp = ialloc(sizeof(SCC), 0);
	scc[nscc] = sp;
	sp->ptr = &dev->ptra;
	sp->data = &dev->dataa;
	sp->freq = freq;
	sccsetup0(sp);
	sp = ialloc(sizeof(SCC), 0);
	scc[nscc+1] = sp;
	sp->ptr = &dev->ptrb;
	sp->data = &dev->datab;
	sp->freq = freq;
	sccsetup0(sp);
	nscc += 2;
}

/*
 *  Queue n characters for output; if queue is full, we lose characters.
 *  Get the output going if it isn't already.
 */
void
sccputc(IOQ *cq, int ch)
{
	SCC *sp = cq->ptr;
	int x;

	x = splhi();
	putc(cq, ch);
	if(sp->printing == 0){
		ch = getc(cq);
		/*kprint("<start %2.2ux>", ch);*/
		if(ch >= 0){
			sp->printing = 1;
			while((*sp->ptr&TxReady)==0)
				;
			*sp->data = ch;
		}
	}
	splx(x);
}

/*
 *  an scc interrupt (a damn lot of work for one character)
 */
void
sccintr0(SCC *sp, uchar x)
{
	int ch;
	IOQ *cq;

	if(x & ExtPendB){
		sccwrreg(sp, 0, ResExtPend);
	}
	if(x & RxPendB){
		cq = sp->iq;
		while(*sp->ptr&RxReady){
			onepointseven();
			ch = *sp->data;
			putc(cq, ch);
		}
	}
	if(x & TxPendB){
		cq = sp->oq;
		ch = getc(cq);
		onepointseven();
		if(ch < 0){
			sccwrreg(sp, 0, ResTxPend);
			sp->printing = 0;
		}else
			*sp->data = ch;
	}
}
void
sccintr(void)
{
	uchar x;
	int i;

	for(i = 0; i < nscc; i += 2){
		x = sccrdreg(scc[i], 3);
		if(x & (ExtPendB|RxPendB|TxPendB))
			sccintr0(scc[i+1], x);
		x = x >> 3;
		if(x & (ExtPendB|RxPendB|TxPendB))
			sccintr0(scc[i], x);
	}
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
void
sccenable(SCC *sp)
{
	/*
	 *  set up i/o routines
	 */
	if(sp->oq){
		sp->oq->ptr = sp;
		sp->sticky[1] |= TxIntEna | ExtIntEna;
	}
	if(sp->iq){
		sp->iq->ptr = sp;
		sp->sticky[1] |= RxIntAllEna | ExtIntEna;
	}

	/*
 	 *  turn on interrupts
	 */
	sccwrreg(sp, 1, 0);
	sp->sticky[9] |= IntEna;
	sccwrreg(sp, 9, 0);

	/*
	 *  turn on DTR and RTS
	 */
	sccdtr(sp, 1);
	sccrts(sp, 1);
}

/*
 *  set up an scc port as something other than a stream
 */
void
sccspecial(int port, IOQ *oq, IOQ *iq, int baud)
{
	SCC *sp = scc[port];

	sp->oq = oq;
	sp->iq = iq;
	sccenable(sp);
	sccsetbaud(sp, baud);
}
