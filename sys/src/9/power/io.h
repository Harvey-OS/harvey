#define UNCACHED	0xA0000000
#define	IO2(t,x)	((t *)(UNCACHED|0x17000000|(x)))
#define VMEA24SUP(t, x)	((t *)(UNCACHED|0x13000000|(x)))
#define VMEA32SUP(t, x)	((t *)(UNCACHED|0x30000000|(x)))
#define	SYNCBUS(t,x)	((t *)(UNCACHED|0x1E000000|(x)))
#define	SBSEM		SYNCBUS(ulong, 0)
#define	SBSEMTOP	SYNCBUS(ulong, 0x400000)

#define	LED		((char*)0xBF200001)
enum
{
	LEDtrapmask=	0xf<<0,
	LEDkfault=	1<<4,
	LEDclock=	1<<5,
	LEDfault=	1<<6,
	LEDpulse=	1<<7,
};
/*
#define LEDON(x) 	(m->ledval &= ~(x), *LED = m->ledval)
#define LEDOFF(x) 	(m->ledval |= (x), *LED = m->ledval)
*/
#define LEDON(x)
#define LEDOFF(x)

typedef struct SBCC	SBCC;
typedef struct Timer	Timer;
typedef struct Duartreg	Duartreg;

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

#define	SBCCREG		SYNCBUS(SBCC, 0x400000)

#define	TIMERREG	SYNCBUS(Timer, 0x1600000)
#define	CLRTIM0		SYNCBUS(uchar, 0x1100000)
#define	CLRTIM1		SYNCBUS(uchar, 0x1180000)

#define	DUARTREG	SYNCBUS(Duartreg, 0x1A00000)

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
extern int ioid;	/* io board type */

typedef struct MODE	MODE;
typedef struct INTVEC	INTVEC;

struct MODE {
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
#define	MASTER	0x1	/* 0x10000000 - Map for cyclone A32 addressing */
#define	SLAVE	0x4
#define	MP2VME(addr)	(((ulong)(addr) & 0x0fffffff) | (SLAVE<<28))
#define	VME2MP(addr)	(((ulong)(addr) & 0x0fffffff) | KZERO)


struct INTVEC {
	struct {
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

#define       PROM_RESET      0   /* run diags, check bootmode, reinit */
#define       PROM_EXEC       1   /* load new program image */
#define       PROM_RESTART    2   /* re-enter monitor command loop */
#define       PROM_REINIT     3   /* re-init monitor, then cmd loop */
#define       PROM_REBOOT     4   /* check bootmode, no config */
#define       PROM_AUTOBOOT   5   /* autoboot the system */

/*
 *  hs datakit board
 */
typedef struct HSdev	HSdev;
struct HSdev {
	ushort	version;
	ushort	pad0x02;
	ushort	vector;
	ushort	pad0x06;
	ushort	csr;
	ushort	pad0x0A;
	ushort	data;
};
#define HSDEV		VMEA24SUP(HSdev, 0xF90000)

