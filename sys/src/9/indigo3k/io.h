#define LIO_ADDR	0x1f000000	/* Local IO Segment */

#define LIGHT_ADDR	0x1f3f0000

#define	GIO0_ADDR	0x1f400000
#define	GIO1_ADDR	0x1f600000

#define MEM_CONF_ADDR0	0x1fa10000	/* memory config register 0 */
#define MEM_CONF_ADDR1	0x1fa10004	/* memory config register 1 */

#define HPC_0_ID_ADDR	0x1fb80000	/* HPC 0 (primary HPC) */

	/* Local I/O Interrupt (INT2) */

#define LIO_0_ISR	0x1fb801c3	/* Local IO interrupt status (b,ro) */
#define LIO_0_MASK	0x1fb801c7	/* Local IO register 0 mask (b) */
#define LIO_1_ISR	0x1fb801cb	/* Local IO interrupt status (b,ro) */
#define LIO_1_MASK	0x1fb801cf	/* Local IO register 1 mask (b) */
#define	TIMER_ACK	0x1fb801e3	/* clear timer (2 bits, ws) */
#define	PT_CLOCK	0x1fb801f0	/* 8254 timer */

#define DUART0		0x1fb80d00	/* kbd, mouse */
#define DUART1		0x1fb80d10	/* external 1,2 */

#define HPC3_RTC	0x1fb80e00	/* Battery backup (real time) clock */
#define HPC3_NVRTC	0x1fb801bf

#define HPC1MEMORY	0x1fbe0000	/* DSP Memory 24 bits x 32k */

#define PROM		0x1fc00000
#define	PROMSIZE	((2*1024*1024)/8)

#define UNMAPPED	0x80000000
#define UNCACHED	0xA0000000
#define	IO(t,x)	((t *)(UNCACHED|LIO_ADDR|(x)))
#define IOC(t,x) ((t *)(UNMAPPED|LIO_ADDR|(x)))

/* LIO 0 status/mask bits */

#define LIO_CENTR	0x02		/* Centronics Printer Interrupt */
#define LIO_SCSI	0x04		/* SCSI interrupt */
#define LIO_ENET	0x08		/* Ethernet interrupt */
#define LIO_DUART	0x20		/* Duart interrupts (OR'd) */
#define LIO_GIO_1	0x40

/* LIO 1 status/mask bits */

#define LIO_DSP		0x10		/* DSP interrupt */

/*
 *  duarts
 */
typedef struct SCCdev	SCCdev;
struct SCCdev
{
	uchar	dummy1[3];
	uchar	ptrb;
	uchar	dummy2[3];
	uchar	datab;
	uchar	dummy3[3];
	uchar	ptra;
	uchar	dummy4[3];
	uchar	dataa;
};

/*
 *  crystal frequency for SCC
 */
#define SCCFREQ	3672000

/*
 * DP8573 real time clock
 */

typedef struct RTCdev	RTCdev;
struct RTCdev
{
	ulong	status;				/* main status reg */
	union{
		struct{
			ulong	dummy0[2];
			ulong	pflag;		/* periodic flag */
			ulong	savctl;		/* time save control */
		};
		struct{
			ulong	rtime;		/* real time mode */
			ulong	outmode;	/* output mode */
			ulong	int0ctl;	/* interrupt control 0 */
			ulong	int1ctl;	/* interrupt control 1 */
		};
	};
	ulong	hsec;				/* sec/100 */
	ulong	sec;
	ulong	min;
	ulong	hour;
	ulong	mday;
	ulong	mon;
	ulong	year;
	ulong	ram0[2];
	ulong	dow;				/* day of week (1-7) */
	ulong	dummy1[4];
	ulong	timcmp[6];			/* time compare ram */
	ulong	timsav[5];			/* time save ram */
	ulong	ram1[2];
};

/*
 * Prom entry points
 */
#define	PROM_REINIT	3	/* re-init monitor, then cmd loop */
#define	PROM_REBOOT	4	/* check bootmode, no config */
