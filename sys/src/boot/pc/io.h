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
	 Vector9=	Int0vec+9,	/*  unassigned */
	 Vector10=	Int0vec+10,	/*  unassigned, usually ethernet */
	 Vector11=	Int0vec+11,	/*  unassigned, usually scsi */
	 Mousevec=	Int0vec+12,	/*  mouse interrupt */
	 Matherr2vec=	Int0vec+13,	/*  math coprocessor */
	 Hardvec=	Int0vec+14,	/*  hard disk */
	 Vector15=	Int0vec+15,	/*  unassigned */

	Syscallvec=	64,
};

enum {
	MaxEISA		= 16,
	EISAconfig	= 0xC80,
};
