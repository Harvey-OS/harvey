/*
 * These register addresses have already been adjusted for BE operation
 *
 * The EISA memory address is completely fictional. Look at mmu.c for
 * a translation of Eisamphys into a 33 bit address.
 */
enum
{
	Devicephys	= 0x80000000,
	Eisaphys	= 0x90000000,	/* IO port physical */
	Devicevirt	= 0xE0000000,
	  MCTgcr	= 0xE0000004,
	Dmabase		= 0xE0000100,	
	  R4030ier	= 0xE00000EC,	/* Interrupt enable register */
	  R4030ipr	= 0xE00000EC,	/* Interrupt pending register */
	  R4030Isr	= 0xE0000204,	/* Interrupt status register */
	  R4030Et	= 0xE000020C,	/* Eisa error type */
	  R4030Rfa	= 0xE000003C,	/* Remote failed address */
	  R4030Mfa	= 0xE0000044,	/* Memory failed address */
	Sonicbase	= 0xE0001000,    	
	Scsibase 	= 0xE0002000,   	
	Floppybase	= 0xE0003000,	
	Clockbase	= 0xE0004000,	
	Uart0		= 0xE0006000,	
	Uart1		= 0xE0007000,	
	  UartFREQ	= 8000000,
	Centronics	= 0xE0008000,
	Nvram		= 0xE0009000, 	
	NvramRW		= 0xE000A000,
	NvramRO		= 0xE000B000,
	 Enetoffset	= 0,
	KeyboardIO	= 0xE0005000,
	 Keyctl		= 6,
	 Keydat		= 7,	
	MouseIO		= 0xE0005000,  	
	SoundIO		= 0xE000C000,
	EisaLatch	= 0xE000E007,
	Diag		= 0xE000F000,
	VideoCTL	= 0x60000000,	
	VideoMEM	= 0x40000000, 	
	   Screenvirt	= 0xE2000000,
	   BTDac	= 0xE2400000,
	I386ack		= 0xE000023f,
	EisaControl	= 0xE0010000,	/* Second 64K Page from Devicevirt */
	  Eisanmi	= EisaControl+0x77,
	Rtcindex	= Eisanmi,
	Rtcdata		= 0xE0004007,
	Eisamphys	= 0x91000000,
	  Eisavgaphys	= Eisamphys+0xA0000,
	Eisamvirt	= 0xE0300000,	/* Second 1M page entry from Intctl */
	Eisadmaintr	= 0x40a,
	Intctlphys	= 0xF0000000,	
	Intctlvirt	= 0xE0200000,
	  Uart1int	= (1<<9),
	  Uart0int	= (1<<8),
	  Mouseint	= (1<<7),
	  Keybint	= (1<<6),
	  Scsiint	= (1<<5),
	  Etherint	= (1<<4),
	  Fiberint	= (1<<3),
	  Soundint	= (1<<2),
	  Floppyint	= (1<<1),
	  Parallelint	= (1<<0),
	Intenareg	= 0xE0200004,	/* Device interrupt enable */
	Intcause	= 0xE0200007,	/* Device interrupt cause register */

	Promvirt	= 0xE1000000,	/* From ARCS chipset */
	Promphys	= 0x1FC00000,	/* MIPs processors */

	Ttbr		= 0xE000001C,	/* Translation table base address */
	Tlrb		= 0xE0000024,	/* Translation table limit address */
	Tir		= 0xE000002C,	/* Translation invalidate register */
	Ntranslation	= 4096,		/* Number of translation registers */
};

enum
{
	SB16pcm		= 100,
};

typedef struct Tte Tte;
struct Tte
{
	ulong	hi;
	ulong	lo;
};

#define IO(type, reg)		(*(type*)(reg))
#define QUAD(type, v)		(type*)(((ulong)(v)+7)&~7)
#define CACHELINE(type, v)	(type*)(((ulong)(v)+127)&~127)
#define UNCACHED(type, v)	(type*)((ulong)(v)|0xA0000000)

#define EISA(v)			(Eisamvirt+(v))
#define EISAINB(port)		(*(uchar*)((EisaControl+(port))^7))
#define EISAINW(port)		(*(ushort*)((EisaControl+(port))^6))
#define EISAOUTB(port, v)	EISAINB(port) = v
#define EISAOUTW(port, v)	EISAINW(port) = v

/*
 *  8259 interrupt controllers
 */
enum
{
	Int0vec=	0,		/* first 8259 */
	 Clockvec=	Int0vec+0,	/*  clock interrupts */
	 Kbdvec=	Int0vec+1,	/*  keyboard interrupts */
	 Uart1vec=	Int0vec+3,	/*  modem line */
	 Uart0vec=	Int0vec+4,	/*  serial line */
	 PCMCIAvec=	Int0vec+5,	/*  PCMCIA card change */
	 Floppyvec=	Int0vec+6,	/*  floppy interrupts */
	 Parallelvec=	Int0vec+7,	/*  parallel port interrupts */
	Int1vec=	Int0vec+8,	/* second 8259 */
	 Ethervec=	Int0vec+10,	/*  ethernet interrupt */
	 Mousevec=	Int0vec+12,	/*  mouse interrupt */
	 Matherr2vec=	Int0vec+13,	/*  math coprocessor */
	 Hardvec=	Int0vec+14,	/*  hard disk */

	Int0ctl=	0x20,		/* control port (ICW1, OCW2, OCW3) */
	Int0aux=	0x21,		/* everything else (ICW2, ICW3, ICW4, OCW1) */
	Int1ctl=	0xA0,		/* control port */
	Int1aux=	0xA1,		/* everything else (ICW2, ICW3, ICW4, OCW1) */

	EOI=		0x20,		/* non-specific end of interrupt */
};

extern int	int0mask;	/* interrupts enabled for first 8259 */
extern int	int1mask;	/* interrupts enabled for second 8259 */

/*
 * All motherboard and ISA interrupts are enabled by default.
 */
#define intrenable(a, b, c, d)
#define BUSUNKNOWN	(-1)
