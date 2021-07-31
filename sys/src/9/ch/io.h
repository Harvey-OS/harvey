/*
 *  All of the following config info is from SGI .h files and represents
 *  what the PROM code passes to the kernel.
 */

#define	IO(t,x)		((t*)(KSEG1|((ulong)x)))

enum
{
	Magic=			0xdeadbabe,
	Serialnumsize=		11,

	Maxios=			8,	/* maximum io boars */
	Maxmc3s=		8,	/* maximum memory boards */
	Maxcpus=		4,	/* max cpus per processor board */
	Maxslots=		16,	/* max slots in backplane */

	Nuart=			6,	/* max uarts on EPC */

	Netheraddr=		6,	/* ethernet address length */
};

enum
{
	/*
	 *  Generic board types and classes
	 */
	BTmask=		0xf0,
	BTnone=		0x00,
	BTcpu=		0x10,
	 BTip19=	 (BTcpu | 0x1),
	 BTip21=	 (BTcpu | 0x2),
	BTio=		0x20,
	 BTio4=		 (BTio | 0x1),
	 BTio5=		 (BTio | 0x2),
	BTmem=		0x30,
	 BTmc3=		 (BTmem | 0x1),

	/*
	 *  adaptor types for the IO4
	 */
	IOAnone=	0x00,
	IOAscsi=	0x0D,
	IOAepc=		0x0E,	
	IOAfchip=	0x0F,
	IOAvmecc=	0x11,
	IOAfcg=		0x20,
	IOAgiocc=	0x2B,
};

/*
 * The ioconfig structure type contains configuration information
 * for the IO adapters on IO4 boards.
 */
typedef struct {
	ulong	pad;
	uchar	enable;		/* Flag indicating whether IOA is enabled */
	uchar	inventory;	/* Previous type info from NVRAM inventory */
	uchar	diagval;	/* Results of diagnostic for this IOA */
	uchar	type;		/* The adapter type */
	uchar	virtid;		/* The virtual ID for this type */
	uchar	subtype;	/* The ID of the device on flat cable */
} Ioconfig;

/*
 * The memconfig structure type contains configuration information
 * for the banks on MC3 boards.
 */
typedef struct {
	ulong	bloc;		/* Bloc num of memory range containing bank */
	uchar	slot;		/* Slot of this bank */
	uchar	enable;		/* Flag indicating bank is enabled */
	uchar	inventory;	/* Previous type info from NVRAM inventory */
	uchar	diagval;	/* Results of diagnostics for the bank */
	uchar	ilp;		/* Bank interleave position */
	uchar	ilf;		/* Bank interleave factor */
	uchar	size;		/* Size of bank */
	uchar	count;		/* Number of banks in this interleave */
} Memconfig;

/*
 * The cpuconfig structure contains configuration information 
 * for the cpus on the IP19 and IP21 boards.
 */
typedef struct {
	ulong	launch;		/* Launch address */
	ulong	parm;		/* Launch parameter */
	uchar	diagloc;	/* Processor status */
	uchar	enable;		/* Flag indicating CPU is enabled */
	uchar 	inventory;	/* Bits indicating inventory status */
	uchar	diagval;	/* Results of diagnostic for this CPU */
	uchar	vpid;		/* Virtual processor ID of the CPU */
	uchar	speed;		/* Processor's frequency (in MHz) */
	uchar	cachesz;	/* log2(Secondary cache size) */
	uchar	promrev;	/* CPU prom revision */
} Cpuconfig;

/*
 *  the configuration for a single board.  SGI believes this structure
 *  to be 108 bytes long.  So does vc.  It had better continue to.
 */
typedef struct {
	union {		/* Union containing board info structures */
		struct {
			Ioconfig	io[Maxios];
			uchar		winnum;		/* IO window assigned */
		};
	
		struct {
			Memconfig	banks[Maxmc3s];
			uchar		mc3num;		/* virtual mc3 id assigned */
		};
	
		struct {
			Cpuconfig	cpus[Maxcpus];
			uchar		cpunum;
			uchar		numcpus;	/* Number of CPUS on board */
		};
	};

	uchar	type;		/* Type of board in this slot */
	uchar	rev;		/* Board revision (if available) */
	uchar	enabled;	/* Flag indicating board is enabled */
	uchar	inventory;	/* Previous type info from NVRAM inventory */
	uchar	diagval;	/* Results of general board diags */
	uchar	slot;		/* Slot containing this board */
} Boardconfig;

/*
 * The Machconfig structure contains all of the configuration information
 * the kernel needs to manage an Everest system.  An instance of this struct 
 * exists at a fixed address in memory (see EVCFGINFO) and is initialized by 
 * the Everest PROM. 
 */
typedef struct {
	Boardconfig	board[Maxslots]; 
	ulong		magic;		/* Magic number for the cfg structure */
	ulong		secs;		/* Clock start time (in secs since 1970) */
	ulong		nanosecs;	/* Clock start time (remainder in ns) */
	ulong		clkfreq;	/* Bus clock frequency (in Hz) */
	ulong 		memsize;	/* Memory size, in 256-byte blocs */
	ulong		epcioa;		/* IO Adapter of master EPC */
	ulong		debugsw;	/* Value of the "virtual DIP switches" when
					   the system was powered on. */
	char		snum[Serialnumsize];	/* Everest serial number */
} Machconfig;
#define	MCONF		((Machconfig*)(KSEG1|0x2000))

/*
 *  address in a 64k window of an IO adapter
 */
#define SWIN(region,padap,off)	((ulong)(0xB0000000|(region<<19)|(padap<<16)|off))

/*
 *  All EPC registers are in its small window
 */
typedef ulong		Address;
extern	Address	epcswin;	/* small window address set by epcinit() */
#define	EPCSWIN(t, x)	((t*)(epcswin | x))

/*
 *  EPC interrupt registers
 */
typedef struct
{
	uvlong	istat;		/* interrupt status */
	uvlong	imset;		/* interrupt mask set */
	uvlong	imreset;	/* interrupt mask reset */
	uvlong	duart0dest;	/* cpu to interrupt for duart 0 */
	uvlong	duart12dest;	/* cpu to interrupt for duarts 1 & 2 */
	uvlong	enetdest;	/* cpu to interrupt for ethernet */
	uvlong	proftimdest;	/* cpu to interrupt for profiling timer */
	uvlong	sparedest;	/* cpu to interrupt for ??? */
	uvlong	paralleldest;	/* cpu to interrupt for parrallel port */
	uvlong	 pad;
	uvlong	errordest;	/* cpu to interrupt for hardware/bus errors */
} EPCintr;
#define EPCINTR	EPCSWIN(EPCintr, 0xa100)

enum
{
	EIduart0=	(1<<0),
	EIduart12=	(1<<1),
	EIenet=		(1<<2),
	EIproftimer=	(1<<3),
	EIspare=	(1<<4),
	EIpport=	(1<<5),
	EIerror=	(1<<7),
};

/*
 *  EPC miscellaneous registers
 */
typedef struct
{
	ulong	 pad0;
	ulong	reset;		/* p dev reset value */
	ulong	 pad1;
	ulong	set;		/* set corresponding p dev reset bits */
	ulong	 pad2;
	ulong	clr;		/* clear corresponding p dev reset */
	uvlong	ibuserr;	/* ibus error */
	uvlong	clribuserr;	/* clear ibus error (by reading) */
	uvlong	errinfo;
} EPCmisc;
#define EPCMISC	EPCSWIN(EPCmisc, 0xa400)

/*
 *  EPC duarts, frequency and registers
 */
#define DUARTFREQ	3672000
typedef struct
{
	uchar	  pad0[7];
	uchar	ctlb;
	uchar	  pad1[7];
	uchar	ctla;
	uchar	  pad2[7];
	uchar	datab;
	uchar	  pad3[7];
	uchar	dataa;
} Duartregs;
#define	EPCDUART0	EPCSWIN(Duartregs, 0x4000)	/* console kbd, mouse */
#define	EPCDUART1	EPCSWIN(Duartregs, 0x5000)	/* 2 rs232 */
#define	EPCDUART2	EPCSWIN(Duartregs, 0x6000)	/* 1 rs232 + 1 rs422 */

/*
 *  CPU board registers.  All must be accessed 64 bits at a time.
 *
 *  For each board, only the first set of per processor board registers
 *  are real.  The 'per processor board' registers for the CPU board in slot
 *  S are CPUregs->conf[S*Maxcpus].  The 'per processor' registers for cpu N
 *  on the CPU board in slot S are CPUregs->conf[S*Maxcpusperboard + N].
 */
typedef struct
{
	/* per processor board */
	uvlong	enablevec;	/* Processor enable vector */
	uvlong	type;		/* Board type */
	uvlong	rev;		/* IP19 board revision level */
	uvlong	urgtimeout;	/* Processor urgent timeout value */
	uvlong	rsctimeout;	/* Chip read resource timeout */
	uvlong	  pad0;
	uvlong	error;		/* Chip error register */
	uvlong	clrerror;	/* Clear on read error register */
	uvlong	  pad1[8];

	/* per processor */
	uvlong	comp0;		/* LSB of the timer comparator */
	uvlong	comp1;		/* Byte 1 of the timer comparator */
	uvlong	comp2;		/* Byte 2 of the timer comparator */
	uvlong	comp3;		/* MSB of the timer comparator */
	uvlong	pgrden;		/* piggyback read enable */
	uvlong	ecchkdis;	/* ECC checking disable register */
	uvlong	cperiod;	/* clock period */
	uvlong	datarate;	/* processor data rate */
	uvlong	  pad2;
	uvlong	wgtimeout;	/* write gatherer retry time out */
	uvlong	  pad3[6];
	uvlong	iwtrig;		/* IW trigger */
	uvlong	rrtrig;		/* RR trigger */
	uvlong	  pad4[28];
	uvlong	cachesz;	/* processor cache size code */
	uvlong	  pad5;
} CPUconf;

typedef struct
{
	uchar	  pad0[8];
	uvlong	slotcpu;	/* (slot<<2)|cpu */
	uvlong	reset;		/* reset the cpu */
	uchar	  pad1[0x800 - 0x18];
	uvlong	ip0;		/* interrupts 63 - 0 */
	uvlong	ip1;		/* interrupts 127 - 64 */
	uchar	  pad2[0x820 - 0x810];
	uvlong	hpil;		/* highest pending interrupt */
	uvlong	cel;		/* current execution level */
	uvlong	cipl0;		/* clear pri 0 interrupts */
	uvlong	igm;		/* interrupt group mask */
	uvlong	ile;		/* interrupt level enable (CPU board generated) */
	uchar	  pad3[0x850 - 0x848];
	uvlong	cipl124;	/* clear pri 1,2,4 interrupts */
	uchar	  pad4[0x900 - 0x858];
	uvlong	ertoip;		/* error/timeout interrupt pending */
	uvlong	certoip;	/* clear ertoip */
	uvlong	eccsb_dis;	/* ECC single bit error disable */
	uchar	  pad5[0xa00 - 0x918];
	uvlong	ro_compare;	/* read only RTC compare */
	uchar	  pad6[0x8000 - 0xa08];
	CPUconf	conf[Maxcpus*Maxslots];
} CPUregs;

#define CPUREGS	((CPUregs*)(KSEG1|0x18000000))
#define	RTC	(uvlong*)(KSEG1|0x18000000|0x20000)

/*
 *  interrupt levels on CPU boards.  we made them the same as SGI for
 *  no particular reason.
 */
enum
{
	ILnet		= 0x3,	/* I don't understand this one -- presotto */
	ILs1		= 0x50,
	ILdang		= 0x64, /* DANG uses 7 intrs, 0x64-0x6a */
	ILplanet	= 0x65,	/* GIO from DANG */
	ILduart0	= 0x6d,
	ILduart12	= 0x6e,
};

typedef struct S1chan S1chan;
struct S1chan
{
	ulong*	dmawrite;	/* address of dma write offset reg */
	ulong*	dmaread;	/* address of dma read offset reg */
	ulong*	dmaxlatlo;	/* low 32 bits of addr xlations */
	ulong*	dmaxlathi;	/* high 8 bits of addr xlations */
	ulong*	dmaflush;	/* dma flush */
	ulong*	dmareset;	/* dma reset */
	ulong	srcmd;		/* status command register */
	ulong	ibuserr;	/* ibus error register */

	ulong	swin;
	int	chan;
};

enum
{
	S1nchan		= 3,

	S1dmaoffset	= 0x2000,
	S1dmawrite	= 0x000,
	S1dmaread	= 0x008,
	S1dmaxlatlo	= 0x010,
	S1dmaxlathi	= 0x018,
	S1dmaflush	= 0x020,
	S1dmareset	= 0x028,
	S1srcmd		= 0x6000,
	S1ibuserr	= 0x7028,
	S1dmainterrupt	= 0x7000,

	/* Status register bits */
	S1srINT2	= (1<<22),
	S1srINT1	= (1<<21),
	S1srINT0	= (1<<20),
	S1srDMA2ON	= (1<<19),
	S1srDMA1ON	= (1<<18),
	S1srDMA0ON	= (1<<17),
	S1srIBERR	= (1<<16),
	S1srPIODrop	= (1<<15),
	S1srINVPIO	= (1<<14),
	S1srMWD		= (1<<13),
	S1srWDOR	= (1<<12),
	S1sr2PIORE	= (1<<11),
	S1sr1PIORE	= (1<<10),
	S1sr0PIORE	= (1<<9),
	S1srEDMA2	= (1<<8),
	S1srEDMA1	= (1<<7),
	S1srEDMA0	= (1<<6),
	S1srENDIAN	= (1<<5),
	S1srSZ		= (1<<4),
	S1srRST2	= (1<<3),
	S1srRST1	= (1<<2),
	S1srRST0	= (1<<1),
};

#define S1DMAON(ch)	(1<<(17+ch))
#define S1RERR(ch)	(1<<(9+ch))
#define S1DERR(ch)	(1<<(6+ch))

enum
{
	GIO_VERSIONREG		= 0x0000,
	GIO_PIOERR		= 0x0000,
	GIO_PIOERRCLR		= 0x0008,
	GIO_BUSERRCMD		= 0x0010,
	GIO_UPPERGIOADDR	= 0x0018,
	GIO_MIDDLEGIOADDR	= 0x0020,
	GIO_BIGENDIAN		= 0x0028,
	GIO_GIO64		= 0x0030,
	GIO_PIPELINED		= 0x0038,
	GIO_GIORESET		= 0x0040,
	GIO_AUDIOACTIVE		= 0x0048,
	GIO_AUDIOSLOT		= 0x0050,
	GIO_PIOWGWRTHRU		= 0x0058,
	GIO_DRIVEGIOPARITY	= 0x0060,
	GIO_IGNGIOTIMEOUT	= 0x0068,
	GIO_CLRTIMEOUT		= 0x0070,
	GIO_MAXGIOTIMEOUT	= 0x0078,
	GIO_DMAMSTATUS		= 0x0800,
	GIO_DMAMERR		= 0x0808,
	GIO_DMAMERRCLR		= 0x0810,
	GIO_DMAMMAXOUTST	= 0x0818,
	GIO_DMAMCACHELINECNT	= 0x0820,
	GIO_DMAMSTART		= 0x0828,
	GIO_DMAMRDTIMEOUT	= 0x0830,
	GIO_DMAMIBUSADDR	= 0x0838,
	GIO_DMAMGIOADDR		= 0x0840,
	GIO_DMAMCOMPLETE	= 0x0848,
	GIO_DMAMCOMPLETECLR	= 0x0850,
	GIO_DMAMKILL		= 0x0858,
	GIO_DMAMPTEADDR		= 0x0860,
	GIO_DMAMIGNGIOPARITY	= 0x0870,
	GIO_DMAMIGNIBUSPARITY	= 0x0878,
	GIO_DMASSTATUS		= 0x1000,
	GIO_DMASERR		= 0x1008,
	GIO_DMASERRCLR		= 0x1010,
	GIO_DMASMAXOUTST	= 0x1018,
	GIO_DMASCACHELINECNT	= 0x1020,
	GIO_DMASRDTIMEOUT	= 0x1030,
	GIO_DMASIBUSADDR	= 0x1038,
	GIO_DMASUPPER8IBUS	= 0x1040,
	GIO_DMASUSEUPPER8	= 0x1048,
	GIO_DMASIGNGIOPARITY	= 0x1058,
	GIO_DMASIGNIBUSPARITY	= 0x1060,
	GIO_INTRSTATUS		= 0x1800,
		GIO_ISRTIMEOUT	= (1<<13),
	GIO_INTRENABLE		= 0x1808,
	GIO_INTRERROR		= 0x1810,
	GIO_INTRGIO0		= 0x1818,
	GIO_INTRGIO1		= 0x1820,
	GIO_INTRGIO2		= 0x1828,
	GIO_INTRDMAMCOMPLETE	= 0x1830,
	GIO_INTRPRIVERR		= 0x1838,
	GIO_INTRLOWATER		= 0x1840,
	GIO_INTRHIWATER		= 0x1848,
	GIO_INTRFULL		= 0x1850,
	GIO_INTRPAUSE		= 0x1858,
	GIO_INTRBREAK		= 0x1860,
	GIO_WGSTATUS		= 0x2000,
	GIO_WGLOWATER		= 0x2008,
	GIO_WGHIWATER		= 0x2010,
	GIO_WGFULL		= 0x2018,
	GIO_WGPRIVERRCLR	= 0x2020,
	GIO_WGPRIVLOADDR	= 0x2028,
	GIO_WGPRIVHIADDR	= 0x2030,
	GIO_WGGIOUPPER		= 0x2038,
	GIO_WGGIOSTREAM		= 0x2040,
	GIO_WGWDCNT		= 0x2048,
	GIO_WGSTREAMWDCNT	= 0x2050,
	GIO_WGFIFORDADDR	= 0x2058,
	GIO_WGPAUSE		= 0x2060,
	GIO_WGSTREAMALWAYS	= 0x2068,
	GIO_WGPOPFIFO		= 0x2070,
	GIO_WGUSTATUS		= 0x3000,
	GIO_WGUWDCNT		= 0x3008,

	DANG_IENMASK		= (1<<0),
	DANG_IERR		= (1<<1),
	DANG_IMMC		= (1<<2),
	DANG_IWGLOW		= (1<<3),
	DANG_IWGHI		= (1<<4),
	DANG_IWGFULL		= (1<<5),
	DANG_IWGPRIV		= (1<<6),
	DANG_IGIO0		= (1<<7),
	DANG_IGIO1		= (1<<8),
	DANG_IGIO2		= (1<<9),
	DANG_IGIOST		= (1<<10),
	DANG_IDMASYNC		= (1<<11),
	DANG_IGFXDLY		= (1<<12),
	DANG_ITIMO		= (1<<13),
};

int	wd95acheck(ulong);
void	wd95aboot(int, S1chan*, ulong);
void	wd95aintr(int);
