#define UNCACHED	0xA0000000
#define LINESIZE	0
#define	IO2(t,x)	((t *)(UNCACHED|0x17000000|(x)))
#define	SYNCBUS(t,x)	((t *)(UNCACHED|0x1E000000|(x)))
#define VMEA16SUP(t, x)	((t *)(UNCACHED|0x17C10000|(x)))
#define VMEA24SUP(t, x)	((t *)(UNCACHED|0x13000000|(x)))
#define VMEA32SUP(t, x)	((t *)(UNCACHED|0x30000000|(x)))

#define	SBSEM		SYNCBUS(ulong, 0)
#define	SBSEMTOP	SYNCBUS(ulong, 0x400000)

#define	LED		((char*)0xBF200001)

typedef struct SBCC	SBCC;
typedef struct Timer	Timer;
typedef struct Duart	Duart;

struct SBCC
{
	ulong	level[14];	/* cpu interrupt level for cpu->cpu ints */
	ulong	junk0[2];
	ulong	status[14];	/* status from other cpu */
	ulong	junk1[2];
	ulong	elevel;		/* cpu interrupt level for vme->cpu ints */
	ulong	junk2[7];
	ulong	flevel;		/* cpu interrupt level for vme->cpu ints */
	ulong	junk3[3];
	ulong	overrun;
	ulong	junk4[3];
	ulong	id;		/* id of this cpu */
	ulong	eintenable;
	ulong	eintpending;
	ulong	fintenable;
	ulong	fintpending;
	ulong	idintenable;
	ulong	idintpending;
	ulong	junk5[8];
	ulong	intxmit;
};

#define NVRAUTHADDR	1924	/* offset into nvram device for keys */
#define	SBCCREG		SYNCBUS(SBCC, 0x400000)

#define	TIMERREG	SYNCBUS(Timer, 0x1600000)
#define	CLRTIM0		SYNCBUS(uchar, 0x1100000)
#define	CLRTIM1		SYNCBUS(uchar, 0x1180000)

#define	DUARTREG	SYNCBUS(Duart, 0x1A00000)

#define LANCERAM	IO2(ushort, 0xE00000)
#define LANCEEND	IO2(ushort, 0xF00000)
#define LANCE3RAM	IO2(ushort, 0xFF4000)
#define LANCE3END	IO2(ushort, 0xFF8000)
#define LANCERDP	IO2(ushort, 0xFC0002)
#define LANCERAP	IO2(ushort, 0xFC000a)
#define LANCEID		IO2(ushort, 0xFF0002)
#define	WRITEMAP	IO2(ulong, 0xFA0000)
#define LANCEINDEX	0x1E00			/* index of lancemap */

#define SCSI0ADDR	IO2(uchar, 0xF08007)
#define SCSI0DATA	IO2(uchar, 0xF08107)
#define SCSI1ADDR	IO2(uchar, 0xF0C007)
#define SCSI1DATA	IO2(uchar, 0xF0C107)
#define SCSI0CNT	IO2(ulong, 0xF54000)
#define SCSI1CNT	IO2(ulong, 0xF58000)
#define SCSI0FLS	IO2(uchar, 0xF30001)
#define SCSI1FLS	IO2(uchar, 0xF34001)

#define IOID		IO2(uchar, 0xFFFFF0)
#define IO2R1		1	/* IO2 revision level 1 */
#define IO2R2		2	/* IO2 revision level 2 */
#define IO3R1		3	/* IO3 revision level 1 */

typedef struct MODE	MODE;
typedef struct INTVEC	INTVEC;

struct	MODE
{
	uchar	masterslave;	/* master/slave addresses for the IO2 */
	uchar	resetforce;
	uchar	checkbits;
	uchar	promenet;
};

#define MODEREG		IO2(MODE, 0xF40000)

/*
 * VME addressing.
 * MP2VME takes a physical MP bus address and returns an address
 * usable by a VME device through A32 space.  VME2MP is its inverse
 */
#define	MASTER	0x1
#define	SLAVE	0x4
#define	MP2VME(addr)	(((ulong)(addr) & 0x0fffffff) | (SLAVE<<28))
#define	VME2MP(addr)	(((ulong)(addr) & 0x0fffffff) | KZERO)

struct	INTVEC
{
	struct
	{
		ulong	vec;
		ulong	fill2;
	} i[8];
};

#define INTVECREG	IO2(INTVEC, 0xF60000)
#define	NVRAM		IO2(uchar, 0xF10000)
#define INTPENDREG	IO2(uchar, 0xF20000)	/* same as LED */
#define INTPENDREG3	IO2(uchar, 0xFF0000)	/* same as ENET ID */
#define IO2CLRMASK	IO2(ulong, 0xFE0000)
#define IO2SETMASK	IO2(ulong, 0xFE8000)
#define	MPBERR0		IO2(ulong, 0xF48000)
#define	MPBERR1		IO2(ulong, 0xF4C000)
#define	RTC		(NVRAM+0x3ff8)

#define SBEADDR		((ulong *)(UNCACHED|0x1F080000))
