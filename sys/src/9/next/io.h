#define	DISPLAYRAM	0x0B000000

/*
 * I/O registers mostly live from 02000000 to 02020000.
 * They are accessed through virtual memory starting at KIO,
 * which maps starting at 02000000.  Other registers are
 * allocated as needed by kmappa().
 */

#define	IO(x)		(x+(KIO-IOMIN))

#define	ISR		IO(0x02007000)
#define	IMR		IO(0x02007800)
#define	SCR1		IO(0x0200C700)
#define	SCR2		IO(0x0200D700)
#define	MONCSR		IO(0x0200E000)
#define	KBDDATA		IO(0x0200E008)
#define	KMSDATA		IO(0x0200E004)
#define	VDMACSR		IO(0x02000180)
#define	NID		IO(0x02006000)
#define ERCVDMA		IO(0x02000150)
#define EXMTDMA		IO(0x02000110)
#define SCSIDMA		IO(0x02000010)

#define	IOMIN		0x02000000
#define	IOMAX		0x02020000

/*
 * Some devices are byte-access and must be accessed through
 * the shadow region to write the proper address.
 * They are accessed through virtual memory starting at KIOB,
 * which maps starting at 02100000.  Other registers are
 * allocated as needed by kmappba().  This distinction is
 * specific to the 68040; you wouldn't need it on the 68030.
 */
#define	IOB(x)		(x+(KIOB-IOBMIN))

#define	VBRIGHTB	IOB(0x02110000)
#define	STCSRB		IOB(0x02116004)
#define	STCLOB		IOB(0x02116001)
#define	STCHIB		IOB(0x02116000)
#define ENETB		IOB(0x02106000)
#define	MEMCTL0		IOB(0x02106010)
#define	MEMCTL1		IOB(0x02106011)
#define	SCCA		IOB(0x02118000)
#define SCSI		IOB(0x02114000)
#define SCSICTL		IOB(0x02114020)
#define SCSIFIFO	IOB(0x02114021)
#define FDCTLRL		IOB(0x02114100)
#define TIMER		IOB(0x02116000)
#define SCCCLKSEL	IOB(0x02118004)

#define	IOBMIN		0x02100000
#define	IOBMAX		0x02120000

#define RDTIMER		(((*(uchar*)TIMER)<<8) | (*(uchar*)(TIMER+1)))

/*
 *  the 16-bit 1MHz count down system timer
 */
typedef struct Systimer	Systimer;
struct Systimer
{
	uchar	high;
	uchar	low;
	uchar	dummy[2];
	uchar	csr;
};

/*
 * SCR2
 */
#define	CCCE	(1<<8)
#define	RTCLK	(1<<9)
#define	RTDATA	(1<<10)

/*
 *  all DMA uses the follwing CSR bit definitions
 */
enum {
	/*
	 *  reading a DMA csr
	 */
	Dberr=		1<<28,		/* bus error */
	Dcint=		1<<27,		/* chain interrupt */
	Dread=		1<<26,		/* DMA is from memory */
	Dchain=		1<<25,		/* chaining DMA in progress */
	Den=		1<<24,		/* DMA in progress */

	/*
	 *  writing a DMA csr
	 */
	Dsetcint=	1<<22,		/* set Dcint */
	Dinit=		1<<21,		/* init DMA buffers */
	Dcreset=	1<<20,		/* turn off end of DMA int */
	Dclrcint=	1<<19,		/* clear Dcint */
	Dsetread=	1<<18,		/* set Dread */
	Dsetchain=	1<<17,		/* set Dchain */
	Dseten=		1<<16,		/* set Den */
};

/*
 *  SCC device
 */
typedef struct SCCdev	SCCdev;
struct SCCdev
{
	uchar	ptrb;
	uchar	ptra;
	uchar	datab;
	uchar	dataa;
};
#define	SCCADDR ((SCCdev *)SCCA)

/*
 *  crystal frequency for SCC
 */
#define SCCFREQ	3684000

/*
 *  SCSI and the floppy controller share a DMA unit.
 *  All of the following is shared by devscsi.c and
 *  devfloppy.c.
 */
typedef struct SCSIdma	SCSIdma;
struct SCSIdma
{
	ulong	csr;
	uchar	pad1[0x4010-0x14];
	ulong	base;
	ulong	limit;
	ulong	chainbase;
	ulong	chainlimit;
};

/* NeXT SCSI control register definitons */
enum
{
	WD_Ncr 		= 1<<0,
	Scsireset 	= 1<<1,
	Fifofl 		= 1<<2,
	Dmadir 		= 1<<3,
	Cpu_dma 	= 1<<4,
	Int_mask 	= 1<<5,
};

/* Manage a software copy of the scsi control hardware register */
uchar _scsic;
#define SETCTL(b)	_scsic |=  (b), *((uchar *)SCSICTL)=_scsic, Xdelay(1)
#define CLRCTL(b)	_scsic &= ~(b), *((uchar *)SCSICTL)=_scsic, Xdelay(1)
#define INITCTL(b)	_scsic  =  (b), *((uchar *)SCSICTL)=_scsic, Xdelay(1)

enum
{
	Oscsi=		0,
	Ofloppy=	1,
};
int dmaowner;
QLock dmalock;

/*
 * DMA audio
 */
#define SOUNDOUTDMA		IO(0x02000040)
#define SOUNDOUTBASE		IO(0x02004040)
#define SOUNDOUTLIMIT		IO(0x02004044)
#define SOUNDOUTCHAINBASE	IO(0x02004048)
#define SOUNDOUTCHAINLIMIT	IO(0x0200404c)

#define SOUNDINDMA		IO(0x02000080)
#define SOUNDINBASE		IO(0x02004080)
#define SOUNDINLIMIT		IO(0x02004084)
#define SOUNDINCHAINBASE	IO(0x02004088)
#define SOUNDINCHAINLIMIT	IO(0x0200408c)
