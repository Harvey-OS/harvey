/*
 * This code runs in rom.
 * Data segment is read only.
 * Set up duart, signon, copy real program into ram and jump to it
 */

extern char end[];
extern memmove(void*, void*, int);

typedef	struct Duart Duart;
typedef unsigned char uchar;
enum
{
	enter		= 0x8000,	/* Address of ram prog to link to */
	Romprg		= 0xF0C02000,	/* Address of program in rom */
	progsize	= 32*1024,	/* Size of prog */
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
delay(long ms)
{
	int i;

	ms = 5000*ms;
	for(i = 0; i < ms; i++)
		;
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
puts(char*s)
{
	while(*s)
		putc(*s++);
}

void
px(long x)
{
	int i;

	puts("0x");
	for(i = 0; i < 8; i++) {
		putc("0123456789abcdef"[(x>>24)&0xf]);
		x = x<<4;
	}
}

void
main(void)
{
	Duart *duart;
	int i, tc;
	void (*call)(void);

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

	puts("\nBOOT\n");

	call = (void(*)(void))enter;
	memmove(call, (void*)Romprg, progsize);

	px(progsize);
	puts(" copy ok, go ");
	px((int)call);
	putc('\n');

	px(((unsigned long*)call)[0]);
	putc('\n');
	px(((unsigned long*)call)[1]);
	putc('\n');
	(*call)();
}
