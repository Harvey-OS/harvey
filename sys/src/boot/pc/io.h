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
	 Floppyvec=	Int0vec+6,	/*  floppy interrupts */
	 Parallelvec=	Int0vec+7,	/*  parallel port interrupts */
	Int1vec=	Int0vec+8,	/* second 8259 */
	 Mousevec=	Int1vec+4,	/*  mouse interrupt */
	 Matherr2vec=	Int1vec+5,	/*  math coprocessor */
	 Hardvec=	Int1vec+6,	/*  hard disk */

	Syscallvec=	64,
};
