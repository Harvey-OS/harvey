#include	<u.h>
#include	"comm.h"
#include	"all.h"
#include	"fns.h"
#include	"io.h"

typedef	struct	Duart	Duart;

enum
{
	DUARTREG	= 0xE0000000,
	Rr0		= 7,
	Wr8		= 0,
	Xreceive	= 1,
	Xempty		= 4,
	Baudrate	= 9600,
	Sccclk		= 16*1000*1000,
};

struct	Duart
{
	uchar	bctl;
	uchar	bdata;
	uchar	actl;
	uchar	adata;
};

static unsigned char cmds[] = 
{
/*	wreg#	value					*/
/*	-----	-----					*/
	0x04,	0x44,	/* 16x clock, 1 stop, no parity	*/
	0x03,	0xc0,	/* Read 8 bit data		*/
	0x05,	0x60,	/* Write 8 bit data no DTR	*/
	0x09,	0x00,	/* No Reset			*/
	0x0a,	0x00,	/* NRZ				*/
	0x0b,	0x50,	/* TxClk = RxClk = Baud Rate Gen*/
	0x0e,	0x02,	/* Baud Rate Generator Source	*/
	0x0e,	0x03,	/* Start Baud Rate Generator	*/
	0x03,	0xc1,	/* Enable Receiver		*/
	0x05,	0x6a,	/* Enable Transmitter		*/
};

void
duartinit(void)
{
	Duart *duart;
	int i, tc;

	duart = (Duart*)DUARTREG;
	delay(10);

	/* Reset the chip */
	duart->actl = 0x00;
	duart->actl = 0x09;
	duart->actl = 0x80;

	/* Set baud rate constants */
	tc = (Sccclk / (2 * Baudrate * 16)) - 2;	/* 16x clock mode! */
	duart->actl = 0x0c;
	duart->actl = tc;				
	duart->actl = 0x0d;
	duart->actl = tc >> 8;

	/* Init registers */			
	for(i = 0; i < sizeof(cmds); i++)
		duart->actl = cmds[i];

	delay(10);
}

void
putc(int c)
{
	Duart *duart;

	if(c == '\n')
		putc('\r');

	duart = (Duart*)DUARTREG;

	for(;;) {
		duart->actl = 0;
		if((duart->actl&Xempty))
			break;
	}
	duart->adata = c;

}

void
chkabort(void)
{
/*
	Taxi *t;
	int csr;
*/
	Duart *duart;

/*
	t = SQL;
	csr = t->csr;
	if(csr & 0x30) 
		print("csr %c%c\n", csr&0x20 ? 'r' : '0', csr&0x20 ? 't' : '0');
*/

	duart = (Duart*)DUARTREG;

	/* Esc aborts the board */
	duart->actl = 0;
	if((duart->actl&Xreceive) == 0)
		return;

	if(duart->adata == 0x1b)
		exits("abort");

}

int
getchar(void)
{
	Duart *duart;

	duart = (Duart*)DUARTREG;

	/* Esc aborts the board */
	duart->actl = 0;
	while((duart->actl&Xreceive) == 0)
		;

	return duart->adata;
}
