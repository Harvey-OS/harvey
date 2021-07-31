/*
 *  programmable interrupt vectors (for the 8259's)
 */
enum
{
	Bptvec=		3,		/* breakpoints */
	Mathemuvec=	7,		/* math coprocessor emulation interrupt */
	Mathovervec=	9,		/* math coprocessor overrun interrupt */
	Matherr1vec=	16,		/* math coprocessor error interrupt */
	Faultvec=	14,		/* page fault */

	Int0vec=	24,		/* first 8259 */
	 Clockvec=	Int0vec+0,	/*  clock interrupts */
	 Kbdvec=	Int0vec+1,	/*  keyboard interrupts */
	 Uart1vec=	Int0vec+3,	/*  modem line */
	 Uart0vec=	Int0vec+4,	/*  serial line */
	 PCMCIAvec=	Int0vec+5,	/*  PCMCIA card change */
	 Floppyvec=	Int0vec+6,	/*  floppy interrupts */
	 Parallelvec=	Int0vec+7,	/*  parallel port interrupts */
	Int1vec=	Int0vec+8,
	 Ethervec=	Int0vec+10,	/*  ethernet interrupt */
	 Mousevec=	Int0vec+12,	/*  mouse interrupt */
	 Matherr2vec=	Int0vec+13,	/*  math coprocessor */
	 ATAvec0=	Int0vec+14,	/*  ATA controller #1 */
	 ATAvec1=	Int0vec+15,	/*  ATA controller #2 */

	Syscallvec=	64,
};

/*
 *  8259 interrupt controllers
 */
enum
{
	Int0ctl=	0x20,		/* control port (ICW1, OCW2, OCW3) */
	Int0aux=	0x21,		/* everything else (ICW2, ICW3, ICW4, OCW1) */
	Int1ctl=	0xA0,		/* control port */
	Int1aux=	0xA1,		/* everything else (ICW2, ICW3, ICW4, OCW1) */

	Icw1=		0x10,		/* select bit in ctl register */
	Ocw2=		0x00,
	Ocw3=		0x08,

	EOI=		0x20,		/* non-specific end of interrupt */
};

extern int	int0mask;	/* interrupts enabled for first 8259 */
extern int	int1mask;	/* interrupts enabled for second 8259 */
