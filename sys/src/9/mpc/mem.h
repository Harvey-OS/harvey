/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2V		8			/* bytes per double word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define ROUND(s, sz)	(((s)+((sz)-1))&~((sz)-1))
#define PGROUND(s)	ROUND(s, BY2PG)
#define	CACHELINELOG	4
#define CACHELINESZ	(1<<CACHELINELOG)
#define BLOCKALIGN	CACHELINESZ

#define	MHz	1000000

#define	MAXMACH		1			/* max # cpus system can run */
#define	MACHSIZE	BY2PG
#define KSTACK		4096			/* Size of kernel stack */

/*
 * Time
 */
#define HZ		100			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((t)*MS2HZ)		/* ticks to milliseconds */
#define	MS2TK(t)	(((t)*HZ+500)/1000)	/* milliseconds to closest tick */

/* Bit encodings for Machine State Register (MSR) */
#define MSR_POW		(1<<18)		/* Enable Power Management */
#define MSR_TGPR	(1<<17)		/* TLB Update registers in use */
#define MSR_ILE		(1<<16)		/* Interrupt Little-Endian enable */
#define MSR_EE		(1<<15)		/* External Interrupt enable */
#define MSR_PR		(1<<14)		/* Supervisor/User privelege */
#define MSR_FP		(1<<13)		/* Floating Point enable */
#define MSR_ME		(1<<12)		/* Machine Check enable */
#define MSR_SE		(1<<10)		/* Single Step */
#define MSR_BE		(1<<9)		/* Branch Trace */
#define MSR_IP		(1<<6)		/* Exception prefix 0x000/0xFFF */
#define MSR_IR		(1<<5)		/* Instruction MMU enable */
#define MSR_DR		(1<<4)		/* Data MMU enable */
#define MSR_RI		(1<<1)		/* Recoverable Exception */
#define MSR_LE		(1<<0)		/* Little-Endian enable */

/*
 * Exception codes (trap vectors)
 */
#define	CRESET	0x01
#define	CMCHECK 0x02
#define	CDSI	0x03
#define	CISI	0x04
#define	CEI	0x05
#define	CALIGN	0x06
#define	CPROG	0x07
#define	CFPU	0x08
#define	CDEC	0x09
#define	CSYSCALL 0x0C
#define	CTRACE	0x0D
#define	CFPA	0x0E
/* rest are power-implementation dependent */
#define	CEMU	0x10
#define	CIMISS	0x11
#define	CDMISS	0x12
#define	CITLBE	0x13
#define	CDTLBE	0x14
#define	CDBREAK	0x1C
#define	CIBREAK	0x1D
#define	CPBREAK	0x1E
#define	CDPORT	0x1F

/*
 * Magic registers
 */

#define	MACH		30		/* R30 is m-> */
#define	USER		29		/* R29 is up-> */


/*
 *  virtual MMU
 */
#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	1984
#define SSEGMAPSIZE	16
#define PPN(x)		((x)&~(BY2PG-1))

/*
 * Fundamental addresses
 */
#define	MACHADDR	(KTZERO-MAXMACH*MACHSIZE)
#define	MACHP(n)	((Mach *)(MACHADDR+(n)*MACHSIZE))

#define	UREGSIZE	((8+32)*4)

/*
 * MMU
 */

/* L1 table entry and Mx_TWC flags */
#define PTEWT		(1<<1)	/* write through */
#define PTE4K		(0<<2)
#define PTE512K		(1<<2)
#define PTE8MB		(3<<2)
#define PTEG		(1<<4)	/* guarded */

/* L2 table entry and Mx_RPN flags (also PTEVALID) */
#define PTECI		(1<<1)	/*  cache inhibit */
#define PTESH		(1<<2)	/* page is shared; ASID ignored */
#define PTELPS		(1<<3)	/* large page size */

#define	PTEKERNEL	(0<<2)
#define	PTEUSER		(1<<2)
#define PTESIZE		(1<<7)

#define	NTLBPID		16
#define	TLBPID(n)	((n)&(NTLBPID-1))

/* soft tlb */
#define	STLBLOG		12
#define	STLBSIZE	(1<<STLBLOG)

/*
 *  portable MMU bits for fault.c - though still machine specific
 */
#define PTEVALID	(MMUPP|MMUV)
#define PTEWRITE	(2<<10)
#define	PTERONLY	(3<<10)
#define	PTEUNCACHED	(1<<4)

/*
 * physical MMU bits
 */

/* Mx_CTR */
#define MMUGPM		(1<<31)	/* group protection mode */
#define	MMUPPM		(1<<30)	/* page protect mode */
#define MMUCIDEF	(1<<29)	/* default CI value when MMU is disabled */
#define MMUWTDEF	(1<<28)	/* default WT value when MMU is disabled */
#define MMURSV4		(1<<27) /* reserve 4 TLB entries */
#define MMUTWAM		(1<<26) /* table walk assist mode */
#define	MMUPPCS		(1<<25) /* privilege/user compare mode */

/* Mx_EPN */
#define MMUEV		(1<<9)	/* TLB entry valid */

/* Mx_TWC */
#define	MMUG		(1<<4)	/* guard */
#define MMUPS4K		(0<<2)	/* 4K or 16K pages */
#define MMUPS512K	(1<<2)	/* 512K pages */
#define MMUPS8M		(3<<2)	/* 8M pages */
#define MMUWT		(1<<1)	/* write through */
#define MMUV		(1<<0)	/* valid */

/* Mx_RPN */
#define MMUPP		(0x1f<<4) /* page protection defaults */
#define MMUSPS		(1<<3)	/* small page size */
#define MMUSH		(1<<2)	/* shared page */
#define MMUCI		(1<<1)	/* cache inhibit */

/*
 *  Address spaces
 *
 *  User is at 0-2GB
 *  Kernel is at 2GB-4GB
 */
#define	UZERO		0			/* base of user address space */
#define	UTZERO		(UZERO+BY2PG)		/* first address in user text */
#define	KZERO		0x80000000		/* base of kernel address space */
#define	KTZERO		0x80010000		/* first address in kernel text */
#define	USTKTOP		(KZERO-BY2PG)		/* byte just beyond user stack */
#define	USTKSIZE	(16*1024*1024)		/* size of user stack */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */
#define TSTKSIZ 	100
#define	CONFPARSED	(KZERO+0x2000)

#define	INTMEM		0xff000000
#define	ISAMEM		0xfe000000
#define	FLASHMEM	0xff200000
#define	SACMEM		FLASHMEM + 0x80000
#define	NVRAMMEM	0xfc000000
#define DRAMMEM		0x80000000

#define	SIRAM	(INTMEM+0xC00)
#define	DPRAM	(INTMEM+0x2000)
#define	DPLEN1	0x200
#define	DPLEN2	0x400
#define	DPLEN3	0x800
#define	DPBASE	(DPRAM+DPLEN3)

#define	SCC1P	(INTMEM+0x3C00)
#define	I2CP	(INTMEM+0x3C80)
#define	MISCP	(INTMEM+0x3CB0)
#define	IDMA1P	(INTMEM+0x3CC0)
#define	SCC2P	(INTMEM+0x3D00)
#define	SPIP	(INTMEM+0x3D80)
#define	TIMERP	(INTMEM+0x3DB0)
#define	IDMA2P	(INTMEM+0x3DC0)
#define	SCC3P	(INTMEM+0x3E00)
#define	SMC1P	(INTMEM+0x3E80)
#define	DSP1P	(INTMEM+0x3EC0)
#define	SCC4P	(INTMEM+0x3F00)
#define	SMC2P	(INTMEM+0x3F80)
#define	DSP2P	(INTMEM+0x3FC0)

#define KEEP_ALIVE_KEY 0x55ccaa33	/* clock and rtc register key */

#define getpgcolor(a)	0
