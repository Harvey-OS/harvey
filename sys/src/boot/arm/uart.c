#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

enum
{
	/* ctl[0] bits */
	Parity=		1<<0,
	Even=		1<<1,
	Stop2=		1<<2,
	Bits8=		1<<3,
	SCE=		1<<4,	/* synchronous clock enable */
	RCE=		1<<5,	/* rx on falling edge of clock */
	TCE=		1<<6,	/* tx on falling edge of clock */

	/* ctl[3] bits */
	Rena=		1<<0,	/* receiver enable */
	Tena=		1<<1,	/* transmitter enable */
	Break=		1<<2,	/* force TXD3 low */
	Rintena=	1<<3,	/* enable receive interrupt */
	Tintena=	1<<4,	/* enable transmitter interrupt */
	Loopback=	1<<5,	/* loop back data */

	/* data bits */
	DEparity=	1<<8,	/* parity error */
	DEframe=	1<<9,	/* framing error */
	DEoverrun=	1<<10,	/* overrun error */

	/* status[0] bits */
	Tint=		1<<0,	/* transmit fifo half full interrupt */
	Rint0=		1<<1,	/* receiver fifo 1/3-2/3 full */
	Rint1=		1<<2,	/* receiver fifo not empty and receiver idle */
	Breakstart=	1<<3,
	Breakend=	1<<4,
	Fifoerror=	1<<5,	/* fifo error */

	/* status[1] bits */
	Tbusy=		1<<0,	/* transmitting */
	Rnotempty=	1<<1,	/* receive fifo not empty */
	Tnotfull=	1<<2,	/* transmit fifo not full */
	ParityError=	1<<3,
	FrameError=	1<<4,
	Overrun=	1<<5,
};

Uartregs *uart3regs = UART3REGS;


/*
 *  for iprint, just write it
 */
void
serialputs(char *str, int n)
{
	Uartregs *ur;

	ur = uart3regs;
	while(n-- > 0){
		/* wait for output ready */
		while((ur->status[1] & Tnotfull) == 0)
			;
		ur->data = *str++;
	}
	while((ur->status[1] & Tbusy))
		;
}
