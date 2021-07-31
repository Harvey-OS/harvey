/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))
#define ICACHESIZE	(64*1024)		/* Power series */

#define	MAXMACH		4			/* max # cpus system can run */

/*
 * Time
 */
#define HZ		100
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((t)*MS2HZ)		/* ticks to milliseconds */
#define	MS2TK(t)	((t)/MS2HZ)		/* milliseconds to ticks */

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
#define IEO		0x00000010
#define KUO		0x00000020
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
#define CM		0x00080000
#define PE		0x00100000
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
#define	MPID		0xBF000000	/* long; low 3 bits identify mp bus slot */
#define WBFLUSH		0xBC000000	/* D-CACHE data; used for write buffer flush */

/*
 * Fundamental addresses
 */

#define	MACHADDR	0x80014000
#define	USERADDR	0xC0000000
#define	UREGADDR	(USERADDR+BY2PG-4-0xA0)
/*
 * MMU
 */

#define	KUSEG	0x00000000
#define KSEG0	0x80000000
#define KSEG1	0xA0000000
#define	KSEG2	0xC0000000
#define	KSEGM	0xE0000000	/* mask to check which seg */

#define	PTEGLOBL	(1<<8)
#define	PTEVALID	(1<<9)
#define	PTEWRITE	(1<<10)
#define PTEUNCACHED	(1<<11)
#define PTERONLY	0
#define	PTEPID(n)	((n)<<6)
#define TLBPID(n)	(((n)>>6)&0x3F)
#define PTEMAPMEM	(2*1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define STLBLOG		13
#define STLBSIZE	(1<<STLBLOG)

#define SEGMAPSIZE	64

#define	NTLBPID	64	/* number of pids */
#define	NTLB	64	/* number of entries */
#define	TLBROFF	8	/* offset of first randomly indexed entry */

/*
 * Address spaces
 */

#define	UZERO		KUSEG			/* base of user address space */
#define	UTZERO		(UZERO+BY2PG)		/* first address in user text */
#define	USTKTOP		KZERO			/* byte just beyond user stack */
#define	TSTKTOP		(USERADDR+USTKSIZE)	/* top of temporary stack */
#define TSTKSIZ 	500
#define	KZERO		KSEG0			/* base of kernel address space */
#define	KTZERO		(KZERO+0x20000)		/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)		/* size of user stack */
#define LKSEGSIZE	(25*BY2PG)
#define LKSEGBASE	(USTKTOP-USTKSIZE-LKSEGSIZE)	
/*
 * Exception codes
 */
#define	CINT	 0		/* external interrupt */
#define	CTLBM	 1		/* TLB modification */
#define	CTLBL	 2		/* TLB miss (load or fetch) */
#define	CTLBS	 3		/* TLB miss (store) */
#define	CADREL	 4		/* address error (load or fetch) */
#define	CADRES	 5		/* address error (store) */
#define	CBUSI	 6		/* bus error (fetch) */
#define	CBUSD	 7		/* bus error (data load or store) */
#define	CSYS	 8		/* system call */
#define	CBRK	 9		/* breakpoint */
#define	CRES	10		/* reserved instruction */
#define	CCPU	11		/* coprocessor unusable */
#define	COVF	12		/* arithmetic overflow */
#define	CUNK13	13		/* undefined 13 */
#define	CUNK14	14		/* undefined 14 */
#define	CUNK15	15		/* undefined 15 */

#define isphys(x) ((ulong)(x) & KZERO)
