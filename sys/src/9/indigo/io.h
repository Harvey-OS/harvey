	/* VME I/O space (mapped by PIC1) */

#define	VME_A32NPBASE	0x10000000	/* a32 non-privileged address sp */
#define	VME_A24SBASE	0x1c000000	/* a24 supervisor address sp */
#define	VME_A16SBASE	0x1d000000	/* a16 supervisor address sp */
#define	VME_A16NPBASE	0x1d100000	/* a16 non-privileged address sp */
#define VME_IACK	0x1df00000	/* VME interrupt acknowledge sp */
#define	VME_A24NPBASE	0x1e000000	/* a24 non-privileged address sp */

#define LIO_ADDR	0x1f000000	/* Local IO Segment */

#define LIGHT_ADDR	0x1f3f0000

#define HPC_3_ID_ADDR	0x1f900000	/* HPC 3 exists */
#define HPC_2_ID_ADDR	0x1f980000	/* HPC 2 exists */

	/* PIC1 */

#define CPU_CONFIG	0x1fa00000	/* Control and status register */
#define SYS_MODE	0x1fa00004	/* System Mode register (ro) */
#define SYSID		0x1fa00008	/* SYSID PROM address (ro) */

#define MEM_CONF_ADDR0	0x1fa10000	/* memory config register 0 */
#define MEM_CONF_ADDR1	0x1fa10004	/* memory config register 1 */
#define REFRESH_TIMER	0x1fa10100	/* timer (wo) */
#define PAR_ERR_ADDR	0x1fa10200	/* Parity error register (ro) */
#define CPU_PAR_ADDR	0x1fa10204	/* CPU Parity error address (ro) */
#define DMA_PAR_ADDR 	0x1fa10208	/* DMA Slave Parity err addr (ro) */
#define PAR_CL_ADDR	0x1fa10210	/* Clear parity err (ws) */

#define GIO_SLOT0_ADDR	0x1fa20000	/* Slot 0 config register */
#define GIO_SLOT1_ADDR	0x1fa20004	/* Slot 1 config register */
#define GIO_BURST_ADDR	0x1fa20008	/* Arbiter Burst register */
#define GIO_DELAY_ADDR	0x1fa2000c	/* Arbiter Delay register */

#define TW_TRIG_ADDR	0x1fa70000	/* 3-way Transfer trigger data */
#define TW_STATUS_ADDR	0x1fa80000	/* 3-way Status (ro) */
#define TW_START_ADDR 	0x1fa80004	/* 3-way Transfer start addr (ro) */
#define TW_MASK_ADDR 	0x1fa80008	/* 3-way Transfer address mask */
#define TW_SUBS_ADDR 	0x1fa8000c	/* 3-way Transfer addr substitute */

#define	VME_RMW_ADDR	0x1fa90000	/* VME RMW toggle (ws) */

#define	GDMA_DABR	0x1faa0000	/* Descriptor array base reg */
#define	GDMA_BUFADR	0x1faa0004	/* Buffer Address reg (ro) */
#define	GDMA_BUFLEN	0x1faa0008	/* Buffer Length reg (ro) */
#define	GDMA_DEST	0x1faa000c	/* Graphics operand address (ro) */
#define	GDMA_STRID	0x1faa0010	/* Stride/line count (ro) */
#define	GDMA_NEXTDESC	0x1faa0014	/* Next descriptor pointer (ro) */
#define	GDMA_START	0x1faa0100	/* Start graphics DMA (ws) */

	/* Ethernet control (HPC1) */

#define HPC_1_ID_ADDR	0x1fb00000	/* HPC 1 exists */

	/* SCSI Control (HPC0).  Always present. */

#define HPC_0_ID_ADDR	0x1fb80000	/* HPC 0 exists (primary HPC) */
#define SCSI0_BC_ADDR	0x1fb80088	/* SCSI byte count register */
#define SCSI0_CBP_ADDR	0x1fb8008c	/* SCSI current buffer pointer */
#define SCSI0_NBDP_ADDR	0x1fb80090	/* SCSI next buffer pointer */
#define SCSI0_CTRL_ADDR	0x1fb80094	/* SCSI control register */
#define SCSI0_PNTR_ADDR	0x1fb80098	/* SCSI pointer register */
#define SCSI0_FIFO_ADDR	0x1fb8009c	/* SCSI fifo data register */
#define PAR_BC_ADDR	0x1fb800a8	/* parallel byte count and IE */
#define PAR_CBP_ADDR	0x1fb800ac	/* parallel current buf ptr */
#define PAR_NBDP_ADDR	0x1fb800b0	/* parallel next buf desc */
#define PAR_CTRL_ADDR	0x1fb800b4	/* parallel control reg */
#define HPC_ENDIAN   	0x1fb800c3	/* dma endian settings (b) */

#define SCSI0A_ADDR	0x1fb80122	/* SCSI WD33C93 address register (b) */
#define SCSI0D_ADDR	0x1fb80126	/* SCSI WD33C93 data register (b) */

#define PAR_SR_ADDR	0x1fb80135	/* parallel status/remote reg (b) */

	/* HDSP-HPC1 registers */

#define HPC1DMAWDCNT	0x1fb80180	/* DMA transfer size (SRAM words) */
#define HPC1GIOADDL	0x1fb80184	/* GIO-bus address, LSB (16 bit) */
#define HPC1GIOADDM	0x1fb80188	/* GIO-bus address, MSB (16 bit) */
#define HPC1PBUSADD	0x1fb8018c	/* PBUS address (16 bit) */
#define HPC1DMACTRL	0x1fb80190	/* DMA Control (2 bit) */
#define HPC1COUNTER	0x1fb80194	/* Counter (24 bits) (ro) */
#define HPC1HANDTX	0x1fb80198	/* Handshake transmit (16 bit) */
#define HPC1HANDRX	0x1fb8019c	/* Handshake receive (16 bit) */
#define HPC1CINTSTAT	0x1fb801a0	/* CPU Interrupt status (3 bit) */
#define HPC1CINTMASK	0x1fb801a4	/* CPU Interrupt masks (3 bit) */
#define HPC1MISCSR	0x1fb801b0	/* Misc. control and status (8 bit) */
#define HPC1BURSTCTL	0x1fb801b4	/* DMA Ballistics register (16 bit) */

#define	CPU_AUX_CONTROL	0x1fb801bf	/* Serial ctrl and LED (b) */

	/* Local I/O Interrupt (INT2) */

#define LIO_0_ISR	0x1fb801c3	/* Local IO interrupt status (b,ro) */
#define LIO_0_MASK	0x1fb801c7	/* Local IO register 0 mask (b) */
#define LIO_1_ISR	0x1fb801cb	/* Local IO interrupt status (b,ro) */
#define LIO_1_MASK	0x1fb801cf	/* Local IO register 1 mask (b) */
#define VME_ISR		0x1fb801d3	/* VME interrupt status (b, ro) */
#define VME_0_MASK	0x1fb801d7	/* VME interrupt mask for LIO 0 (b) */
#define VME_1_MASK	0x1fb801db	/* VME interrupt mask for LIO 1 (b) */
#define	PORT_CONFIG	0x1fb801df	/* LEDs (5 bits) */
#define	TIMER_ACK	0x1fb801e3	/* clear timer (2 bits, ws) */
#define	PT_CLOCK	0x1fb801f0	/* 8254 timer */

#define DUART0		0x1fb80d00	/* kbd, mouse */
#define DUART1		0x1fb80d10	/* external 1,2 */
#define DUART2		0x1fb80d20	/* ? */
#define DUART3		0x1fb80d30	/* ? */

#define RT_CLOCK	0x1fb80e00	/* Battery backup (real time) clock */

#define BOARD_REV	0x1fbd0000	/* Board revision - modem SRAM space */

#define HPC1MEMORY	0x1fbe0000	/* DSP Memory 24 bits x 32k */

#define PROM		0x1fc00000

#define UNMAPPED	0x80000000
#define UNCACHED	0xA0000000
#define	IO(t,x)	((t *)(UNCACHED|LIO_ADDR|(x)))
#define IOC(t,x) ((t *)(UNMAPPED|LIO_ADDR|(x)))

/* LIO 0 status/mask bits */

#define LIO_FIFO	0x01		/* FIFO full / GIO-0  interrupt */
#define LIO_GIO_0	0x01
#define LIO_CENTR	0x02		/* Centronics Printer Interrupt */
#define LIO_SCSI	0x04		/* SCSI interrupt */
#define LIO_ENET	0x08		/* Ethernet interrupt */
#define LIO_GDMA	0x10		/* Graphics dma done interrupt */
#define LIO_DUART	0x20		/* Duart interrupts (OR'd) */
#define LIO_GE		0x40		/* GE/GIO-1/second HPC1 interrupt */
#define LIO_GIO_1	0x40
#define LIO_VME0	0x80		/* VME-0 interrupt */

/* LIO 1 status/mask bits */

#define LIO_BIT0_UNUSED	0x01
#define LIO_GR1MODE	0x02		/* GR1 Mode(IP12) */
#define LIO_BIT2_UNUSED	0x04
#define LIO_VME1	0x08		/* VME-1 interrupt */
#define LIO_DSP		0x10		/* DSP interrupt */
#define LIO_AC		0x20		/* ACFAIL interrupt */
#define LIO_VIDEO	0x40		/* Video option interrupt */
#define LIO_VR		0x80		/* Vert retrace / GIO-2 interrupt */
#define LIO_GIO_2	0x80

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
 * 8254 programmable interval timer
 *
 * Counter2 is clocked at MASTER_FREQ (defined below).  The
 * output of counter2 is the clock for both counter0 and counter1.
 * Counter0 output is tied to INTR3; counter1 output is tied to
 * INTR4.
 */

#define MASTER_FREQ	1000000

typedef struct Pt_clock	Pt_clock;
struct Pt_clock
{
	uchar	fill0[3];
	uchar	c0;		/* counter 0 port */
	uchar	fill1[3];
	uchar	c1;		/* counter 1 port */
	uchar	fill2[3];
	uchar	c2;		/* counter 2 port */
	uchar	fill3[3];
	uchar	cw;		/* control word */
};

/*
 * Control words
 */
#define	PTCW_SC(x)	((x)<<6)	/* select counter x */
#define	PTCW_RBCMD	(3<<6)		/* read-back command */
#define	PTCW_CLCMD	(0<<4)		/* counter latch command */
#define	PTCW_LSB	(1<<4)		/* r/w least signif. byte only */
#define	PTCW_MSB	(2<<4)		/* r/w most signif. byte only */
#define	PTCW_16B	(3<<4)		/* r/w 16 bits, lsb then msb */
#define	PTCW_BCD	0x01		/* operate in BCD mode */

/*
 * Modes
 */
#define	MODE_ITC	(0<<1)		/* interrupt on terminal count */
#define	MODE_HROS	(1<<1)		/* hw retriggerable one-shot */
#define	MODE_RG		(2<<1)		/* rate generator */
#define	MODE_SQW	(3<<1)		/* square wave generator */
#define	MODE_STS	(4<<1)		/* software triggered strobe */
#define	MODE_HTS	(5<<1)		/* hardware triggered strobe */

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

#define	PROM_RESET	0	/* run diags, check bootmode, reinit */
#define	PROM_EXEC	1	/* load new program image */
#define	PROM_RESTART	2	/* re-enter monitor command loop */
#define	PROM_REINIT	3	/* re-init monitor, then cmd loop */
#define	PROM_REBOOT	4	/* check bootmode, no config */
#define	PROM_AUTOBOOT	5	/* autoboot the system */
