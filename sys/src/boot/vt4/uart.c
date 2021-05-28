/* xilinx uartlite driver (polling) */
#include "include.h"

enum {
	/* UART Lite register offsets */
	Rxfifo		= 0,		/* receive FIFO, read only */
	Txfifo		= 4,		/* transmit FIFO, write only */
	Status		= 8,		/* status register, read only */
	Ctl		= 12,		/* control reg, write only */

	/* control register bit positions */
	Ctlintrena	= 0x10,		/* enable interrupt */
	Rxfiforst	= 0x02,		/* reset receive FIFO */
	Txfiforst	= 0x01,		/* reset transmit FIFO */

	/* status register bit positions */
	Parerr		= 0x80,
	Framerr		= 0x40,
	Overrunerr	= 0x20,
	Stsintrena	= 0x10,		/* interrupt enabled */
	Txfifofull	= 0x08,		/* transmit FIFO full */
	Txfifoempty	= 0x04,		/* transmit FIFO empty */
	Rxfifofull	= 0x02,		/* receive FIFO full */
	Rxfifohasdata	= 0x01,		/* data in receive FIFO */

	FIFO_SIZE	= 16,		/* bytes in tx or rx fifo */
};

#define Uartctl		((ulong *)(Uartlite + Ctl))
#define Uartstatus	((ulong *)(Uartlite + Status))
#define Uartrxfifo	((ulong *)(Uartlite + Rxfifo))
/* #define Uarttxfifo	((ulong *)(Uartlite + Txfifo)) /* see prototype.h */

void
vuartputc(int c)
{
	int i;

	coherence();
	i = 2000000;		/* allow > 1.04 ms per char @ 9600 baud */
	while(*Uartstatus & Txfifofull && i-- > 0)
		;
	*Uarttxfifo = (uchar)c;
	coherence();
}

int
vuartputs(char *s, int len)
{
	while (len-- > 0) {
		if (*s == '\n')
			vuartputc('\r');
		vuartputc(*s++);
	}
	return len;
}

int
vuartgetc(void)
{
	while((*Uartstatus & Rxfifohasdata) == 0)
		;
	return (uchar)*Uartrxfifo;
}
