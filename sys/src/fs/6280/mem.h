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
#define	MS2HZ		10		/* millisec per clock tick */
#define	TK2MS(t)	((t)*MS2HZ)	/* ticks to milliseconds */

#define	MAXMACH		1		/* max # cpus system can run */

/*
 * CP0 registers
 */

#define INDEX		0
#define RANDOM		1
#define TLBPHYS		2
#define CONTEXT		4
#define ERROR		7		/* R6000 */
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
#define MM		0x00010000	/* R6000 memory management */
#define CU1		0x20000000

/*
 * Traps
 */
#define	UTLBMISS	(KSEG0+0x00)
#define	EXCEPTION	(KSEG0+0x80)
#define WBACK2LOOP	(KSEG0+0x100)
#define INVAL2LOOP	(KSEG0+0x200)
#define CACHEFLOOP	(KSEG0+0x300)

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

#define	PTEGLOBL	(1<<8)
#define	PTEVALID	(1<<9)
#define	PTEWRITE	(1<<10)
#define	PTEPID(n)	((n)<<6)

#define	NTLBPID	64			/* number of pids */
#define	NTLB	64			/* number of entries */
#define	TLBROFF	8			/* offset of first randomly indexed entry */

/*
 * Address spaces
 */

#define	UZERO	KUSEG			/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	USTKTOP	KZERO			/* byte just beyond user stack */
#define	KZERO	KSEG0			/* base of kernel address space */
#define	KTZERO	(KSEG0+0x20000)		/* first address in kernel text */

/*
 * Exception codes
 */
#define	CINT	 0			/* external interrupt */
#define	CTLBM	 1			/* TLB modification */
#define	CTLBL	 2			/* TLB miss (load or fetch) */
#define	CTLBS	 3			/* TLB miss (store) */
#define	CADREL	 4			/* address error (load or fetch) */
#define	CADRES	 5			/* address error (store) */
#define	CBUSI	 6			/* bus error (fetch) */
#define	CBUSD	 7			/* bus error (data load or store) */
#define	CSYS	 8			/* system call */
#define	CBRK	 9			/* breakpoint */
#define	CRES	10			/* reserved instruction */
#define	CCPU	11			/* coprocessor unusable */
#define	COVF	12			/* arithmetic overflow */
#define	CTR	13			/* R6000 trap exception */
#define	CNCD	14			/* R6000 MM to uncached address */
#define	CMC	15			/* R6000 machine check */
#define NEXCODE	32			/* R6000 field in CAUSE is 5 bits */

#define	NSEG	5
