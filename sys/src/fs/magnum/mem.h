/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8		/* bits per byte */
#define BI2WD		32		/* bits per word */
#define	BY2WD		4		/* bytes per word */
#define	BY2PG		16384		/* R6000 bytes per page */
#define	WD2PG		(BY2PG/BY2WD)	/* log(BY2PG) */
#define	HZ		(100)		/* clock frequency */
#define	MS2HZ		(1000/HZ)	/* millisec per clock tick */
#define	TK2MS(t)	((t)*MS2HZ)	/* ticks to milliseconds */
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

#define	MAXMACH		1		/* max # cpus system can run */

/*
 * CP0 registers
 */

#define INDEX		0
#define RANDOM		1
#define TLBPHYS		2
#define CONTEXT		4
#define BADVADDR	8
#define TLBVIRT		10
#define STATUS		12
#define CAUSE		13
#define EPC		14
#define	PRID		15

/*
 * M(STATUS) bits
 */
#define IEC		0x00000001
#define KUC		0x00000002
#define IEP		0x00000004
#define KUP		0x00000008
#define INTMASK		0x0000ff00
#define SW0		0x00000100
#define SW1		0x00000200
#define INTR0		0x00000400
#define INTR1		0x00000800
#define INTR2		0x00001000
#define INTR3		0x00002000
#define INTR4		0x00004000
#define INTR5		0x00008000
#define ISC		0x00010000
#define SWC		0x00020000
#define CU1		0x20000000

/*
 * Traps
 */
#define	UTLBMISS	(KSEG0+0x00)
#define	EXCEPTION	(KSEG0+0x80)

/*
 * Magic registers
 */

#define	MACH		25		/* R25 is m-> */
#define	USER		24		/* R24 is u-> */

/*
 * Fundamental addresses
 */
#define	MACHADDR	0x80014000

/*
 * MMU
 */

#define	KUSEG	0x00000000
#define KSEG0	0x80000000
#define KSEG1	0xA0000000
#define	KSEG2	0xC0000000

/*
 * Address spaces
 */
#define	KZERO	KSEG0			/* base of kernel address space */
#define	KTZERO	(KSEG0+0x20000)		/* first address in kernel text */

/*
 * Exception codes
 */
#define	CINT	0x00			/* external interrupt */
#define	CTLBM	0x01			/* TLB modification */
#define	CTLBL	0x02			/* TLB miss (load or fetch) */
#define	CTLBS	0x03			/* TLB miss (store) */
#define	CADREL	0x04			/* address error (load or fetch) */
#define	CADRES	0x05			/* address error (store) */
#define	CBUSI	0x06			/* bus error (fetch) */
#define	CBUSD	0x07			/* bus error (data load or store) */
#define	CSYS	0x08			/* system call */
#define	CBRK	0x09			/* breakpoint */
#define	CRES	0x0A			/* reserved instruction */
#define	CCPU	0x0B			/* coprocessor unusable */
#define	COVF	0x0C			/* arithmetic overflow */
#define	EXCMASK	0x0F			/* mask of all causes */

#define	NTLB	64	/* number of entries */
