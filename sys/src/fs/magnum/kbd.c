#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

enum
{
	/*
	 *  commands
	 */
	Krdcmd=		0x20,	/* read command byte */
	Kwrcmd=		0x60,	/* write command byte */
	Kselftest=	0xAA,	/* self test */
	Ktest=		0xAB,	/* keyboard test */
	Kdisable=	0xAD,	/* disable keyboard */
	Kenable=	0xAE,	/* enable keyboard */
	Krdin=		0xC0,	/* read input port */
	Krdout=		0xD0,	/* read output port */
	Kwrout=		0xD1,	/* write output port */
	Krdtest=	0xE0,	/* read test inputs */
	Kwrlights=	0xED,	/* set lights */
	Kreset=		0xF0,	/* soft reset */
	/*
	 *  magic characters
	 */
	Msetscan=	0xF0,	/* set scan type (0 == unix) */
	Menable=	0xF4,	/* enable the keyboard */
	Mdisable=	0xF5,	/* disable the keyboard */
	Mdefault=	0xF6,	/* set defaults */
	Mreset=		0xFF,	/* reset the keyboard */
	/*
	 *  responses from keyboard
	 */
	Rok=		0xAA,		/* self test OK */
	Recho=		0xEE,		/* ??? */
	Rack=		0xFA,		/* command acknowledged */
	Rfail=		0xFC,		/* self test failed */
	Rresend=	0xFE,		/* ??? */
	Rovfl=		0xFF,		/* input overflow */
	/*
	 *  command register bits
	 */
	Cintena=	1<<0,	/* enable output interrupt */
	Csystem=	1<<2,	/* set system */
	Cinhibit=	1<<3,	/* inhibit override */
	Cdisable=	1<<4,	/* disable keyboard */
	/*
	 *  output port bits
	 */
	Osoft=		1<<0,	/* soft reset bit (must be 1?) */
	Oparity=	1<<1,	/* force bad parity */
	Omemtype=	1<<2,	/* simm type (1 = 4Mb, 0 = 1Mb)	*/
	Obigendian=	1<<3,	/* big endian */
	Ointena=	1<<4,	/* enable interrupt */
	Oclear=		1<<5,	/* clear expansion slot interrupt */
	/*
	 *  status bits
	 */
	Sobf=		1<<0,	/* output buffer full */
	Sibf=		1<<1,	/* input buffer full */
	Ssys=		1<<2,	/* set by self-test */
	Slast=		1<<3,	/* last access was to data */
	Senabled=	1<<4,	/* keyboard is enabled */
	Stxtimeout=	1<<5,	/* transmit to kybd has timed out */
	Srxtimeout=	1<<6,	/* receive from kybd has timed out */
	Sparity=	1<<7,	/* parity on byte was even */
	/*
	 *  light bits
	 */
	L1=		1<<0,	/* light 1, network activity */
	L2=		1<<2,	/* light 2, caps lock */
	L3=		1<<1,	/* light 3, no label */
};

#define OUTWAIT	while(k->ctl & Sibf); Xdelay(20)
#define INWAIT	while(!(k->ctl & Sobf)); Xdelay(20)
#define ACKWAIT INWAIT ; if(k->data != Rack) print("bad response\n"); Xdelay(20)

void
Xdelay(int us)
{
	int i;

	us *= 7;			/* experimentally determined */
	for(i=0; i<us; i++)
		;
}

static void
empty(void)
{
	KBDdev *k = KBD;
	int i;

	/*
	 *  empty the buffer
	 */
	Xdelay(20);
	while(k->ctl & Sobf){
		i = k->data;
		USED(i);
		Xdelay(1);
	}
}

int
setsimmtype(int type)
{
	KBDdev *k = KBD;
	uchar data;

	empty();

	/*
	 * read current value and mask out
	 * the SIMM type
	 */
	OUTWAIT;
	k->ctl = Krdout;
	INWAIT;
	data = (k->data & ~Omemtype);

	/*
	 * write out with the new value
	 */
	if(type == 4)
		data |= Omemtype;
	OUTWAIT;
	k->ctl = Kwrout;
	OUTWAIT;
	k->data = data;

	empty();
	/*
	 * let things settle
	 */
	delay(10);
	return data;
}
