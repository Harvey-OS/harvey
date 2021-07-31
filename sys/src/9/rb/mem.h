/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2V		8			/* bytes per vlong */

#define MAXBY2PG (16*1024) /* rounding for UTZERO in executables; see mkfile */
#define UTROUND(t)	ROUNDUP((t), MAXBY2PG)

#ifndef BIGPAGES
#define	BY2PG		4096			/* bytes per page */
#define	PGSHIFT		12			/* log2(BY2PG) */
#define	PGSZ		PGSZ4K
#define MACHSIZE	(2*BY2PG)
#else
/* 16K pages work very poorly */
#define	BY2PG		(16*1024)		/* bytes per page */
#define	PGSHIFT		14			/* log2(BY2PG) */
#define PGSZ		PGSZ16K
#define MACHSIZE	BY2PG
#endif

#define	KSTACK		8192			/* Size of kernel stack */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */

#define	MAXMACH		1   /* max # cpus system can run; see active.machs */
#define STACKALIGN(sp)	((sp) & ~7)		/* bug: assure with alloc */
#define	BLOCKALIGN	16
#define CACHELINESZ	32			/* mips24k */
#define ICACHESIZE	(64*1024)		/* rb450g */
#define DCACHESIZE	(32*1024)		/* rb450g */

#define MASK(w)		FMASK(0, w)

/*
 * Time
 */
#define	HZ		100			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 * CP0 registers
 */

#define INDEX		0
#define RANDOM		1
#define TLBPHYS0	2	/* aka ENTRYLO0 */
#define TLBPHYS1	3	/* aka ENTRYLO1 */
#define CONTEXT		4
#define PAGEMASK	5
#define WIRED		6
#define BADVADDR	8
#define COUNT		9
#define TLBVIRT		10	/* aka ENTRYHI */
#define COMPARE		11
#define STATUS		12
#define CAUSE		13
#define EPC		14
#define	PRID		15
#define	CONFIG		16
#define	LLADDR		17
#define	WATCHLO		18
#define	WATCHHI		19
#define DEBUG		23
#define DEPC		24
#define PERFCOUNT	25
#define	CACHEECC	26
#define	CACHEERR	27
#define	TAGLO		28
#define	TAGHI		29
#define	ERROREPC	30
#define DESAVE		31

/*
 * M(STATUS) bits
 */
#define KMODEMASK	0x0000001f
#define IE		0x00000001	/* master interrupt enable */
#define EXL		0x00000002	/* exception level */
#define ERL		0x00000004	/* error level */
#define KSUPER		0x00000008
#define KUSER		0x00000010
#define KSU		0x00000018
//#define UX		0x00000020 /* no [USK]X 64-bit extension bits on 24k */
//#define SX		0x00000040
//#define KX		0x00000080
#define INTMASK		0x0000ff00
#define INTR0		0x00000100	/* interrupt enable bits */
#define INTR1		0x00000200
#define INTR2		0x00000400
#define INTR3		0x00000800
#define INTR4		0x00001000
#define INTR5		0x00002000
#define INTR6		0x00004000
#define INTR7		0x00008000
//#define DE		0x00010000	/* not on 24k */
#define TS		0x00200000	/* tlb shutdown; on 24k at least */
#define BEV		0x00400000	/* bootstrap exception vectors */
#define RE		0x02000000	/* reverse-endian in user mode */
#define FR		0x04000000	/* enable 32 FP regs */
#define CU0		0x10000000
#define CU1		0x20000000	/* FPU enable */

/*
 * M(CONFIG) bits
 */

#define CFG_K0		7	/* kseg0 cachability */
#define CFG_MM		(1<<18)	/* write-through merging enabled */

/*
 * M(CAUSE) bits
 */

#define BD		(1<<31)	/* last excep'n occurred in branch delay slot */

/*
 * Exception codes
 */
#define	EXCMASK	0x1f		/* mask of all causes */
#define	CINT	 0		/* external interrupt */
#define	CTLBM	 1		/* TLB modification: store to unwritable page */
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
#define	CTRAP	13		/* trap */
#define	CVCEI	14		/* virtual coherence exception (instruction) */
#define	CFPE	15		/* floating point exception */
#define CTLBRI	19		/* tlb read-inhibit */
#define CTLBXI	20		/* tlb execute-inhibit */
#define	CWATCH	23		/* watch exception */
#define CMCHK	24		/* machine checkcore */
#define CCACHERR 30		/* cache error */
#define	CVCED	31		/* virtual coherence exception (data) */

/*
 * M(CACHEECC) a.k.a. ErrCtl bits
 */
#define PE	(1<<31)
#define LBE	(1<<25)
#define WABE	(1<<24)

/*
 * Trap vectors
 */

#define	UTLBMISS	(KSEG0+0x000)
#define	XEXCEPTION	(KSEG0+0x080)
#define	CACHETRAP	(KSEG0+0x100)
#define	EXCEPTION	(KSEG0+0x180)

/*
 * Magic registers
 */

#define	USER		24		/* R24 is up-> */
#define	MACH		25		/* R25 is m-> */

/*
 * offsets in ureg.h for l.s
 */
#define	Ureg_status	(Uoffset+0)
#define	Ureg_pc		(Uoffset+4)
#define	Ureg_sp		(Uoffset+8)
#define	Ureg_cause	(Uoffset+12)
#define	Ureg_badvaddr	(Uoffset+16)
#define	Ureg_tlbvirt	(Uoffset+20)

#define	Ureg_hi		(Uoffset+24)
#define	Ureg_lo		(Uoffset+28)
#define	Ureg_r31	(Uoffset+32)
#define	Ureg_r30	(Uoffset+36)
#define	Ureg_r28	(Uoffset+40)
#define	Ureg_r27	(Uoffset+44)
#define	Ureg_r26	(Uoffset+48)
#define	Ureg_r25	(Uoffset+52)
#define	Ureg_r24	(Uoffset+56)
#define	Ureg_r23	(Uoffset+60)
#define	Ureg_r22	(Uoffset+64)
#define	Ureg_r21	(Uoffset+68)
#define	Ureg_r20	(Uoffset+72)
#define	Ureg_r19	(Uoffset+76)
#define	Ureg_r18	(Uoffset+80)
#define	Ureg_r17	(Uoffset+84)
#define	Ureg_r16	(Uoffset+88)
#define	Ureg_r15	(Uoffset+92)
#define	Ureg_r14	(Uoffset+96)
#define	Ureg_r13	(Uoffset+100)
#define	Ureg_r12	(Uoffset+104)
#define	Ureg_r11	(Uoffset+108)
#define	Ureg_r10	(Uoffset+112)
#define	Ureg_r9		(Uoffset+116)
#define	Ureg_r8		(Uoffset+120)
#define	Ureg_r7		(Uoffset+124)
#define	Ureg_r6		(Uoffset+128)
#define	Ureg_r5		(Uoffset+132)
#define	Ureg_r4		(Uoffset+136)
#define	Ureg_r3		(Uoffset+140)
#define	Ureg_r2		(Uoffset+144)
#define	Ureg_r1		(Uoffset+148)

/* ch and carrera used these defs */
	/* Sizeof(Ureg) + (R5,R6) + 16 bytes slop + retpc + ur */
// #define UREGSIZE ((Ureg_r1+4-Uoffset) + 2*BY2V + 16 + BY2WD + BY2WD)
// #define Uoffset	8

// #define UREGSIZE	(Ureg_r1 + 4 - Uoffset)	/* this ought to work */
#define UREGSIZE ((Ureg_r1+4-Uoffset) + 2*BY2V + 16 + BY2WD + BY2WD)
#define Uoffset		0
#define Notuoffset	8

/*
 * MMU
 */
#define	PGSZ4K		(0x00<<13)
#define PGSZ16K		(0x03<<13)	/* on 24k */
#define	PGSZ64K		(0x0F<<13)
#define	PGSZ256K	(0x3F<<13)
#define	PGSZ1M		(0xFF<<13)
#define	PGSZ4M		(0x3FF<<13)
// #define PGSZ8M	(0x7FF<<13)	/* not on 24k */
#define	PGSZ16M		(0xFFF<<13)
#define PGSZ64M		(0x3FFF<<13)	/* on 24k */
#define PGSZ256M	(0xFFFF<<13)	/* on 24k */

/* mips address spaces, tlb-mapped unless marked otherwise */
#define	KUSEG	0x00000000	/* user process */
#define KSEG0	0x80000000	/* kernel (direct mapped, cached) */
#define KSEG1	0xA0000000	/* kernel (direct mapped, uncached: i/o) */
#define	KSEG2	0xC0000000	/* kernel, used for TSTKTOP */
#define	KSEG3	0xE0000000	/* kernel, used by kmap */
#define	KSEGM	0xE0000000	/* mask to check which seg */

/*
 * Fundamental addresses
 */

#define	REBOOTADDR	KADDR(0x1000)	/* just above vectors */
#define	MACHADDR	0x80005000	/* Mach structures */
#define	MACHP(n)	((Mach *)(MACHADDR+(n)*MACHSIZE))
#define ROM		0xbfc00000
#define	KMAPADDR	0xE0000000	/* kmap'd addresses */
#define	WIREDADDR	0xE2000000	/* address wired kernel space */

#define PHYSCONS	(KSEG1|0x18020000)		/* i8250 uart */

#define PIDXSHFT	12
#ifndef BIGPAGES
#define NCOLOR		8
#define PIDX		((NCOLOR-1)<<PIDXSHFT)
#define getpgcolor(a)	(((ulong)(a)>>PIDXSHFT) % NCOLOR)
#else
/* no cache aliases are possible with pages of 16K or larger */
#define NCOLOR		1
#define PIDX		0
#define getpgcolor(a)	0
#endif
#define KMAPSHIFT	15

#define	PTEGLOBL	(1<<0)
#define	PTEVALID	(1<<1)
#define	PTEWRITE	(1<<2)
#define PTERONLY	0
#define PTEALGMASK	(7<<3)
#define PTENONCOHERWT	(0<<3)		/* cached, write-through (slower) */
#define PTEUNCACHED	(2<<3)
#define PTENONCOHERWB	(3<<3)		/* cached, write-back */
#define PTEUNCACHEDACC	(7<<3)
/* rest are reserved on 24k */
#define PTECOHERXCL	(4<<3)
#define PTECOHERXCLW	(5<<3)
#define PTECOHERUPDW	(6<<3)

/* how much faster is it? mflops goes from about .206 (WT) to .37 (WB) */
#define PTECACHABILITY PTENONCOHERWT	/* 24k erratum 48 disallows WB */
// #define PTECACHABILITY PTENONCOHERWB

#define	PTEPID(n)	(n)
#define PTEMAPMEM	(1024*1024)
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	512
#define SSEGMAPSIZE	16

#define STLBLOG		15
#define STLBSIZE	(1<<STLBLOG)	/* entries in the soft TLB */
/* page # bits that don't fit in STLBLOG bits */
#define HIPFNBITS	(BI2WD - (PGSHIFT+1) - STLBLOG)
#define KPTELOG		8
#define KPTESIZE	(1<<KPTELOG)	/* entries in the kfault soft TLB */

#define TLBPID(n) ((n)&0xFF)
#define	NTLBPID	256		/* # of pids (affects size of Mach) */
#define	NTLB	16		/* # of entries (mips 24k) */
#define TLBOFF	1		/* first tlb entry (0 used within mmuswitch) */
#define NKTLB	2		/* # of initial kfault tlb entries */
#define WTLBOFF	(TLBOFF+NKTLB)	/* first large IO window tlb entry */
#define NWTLB	0		/* # of large IO window tlb entries */
#define	TLBROFF	(WTLBOFF+NWTLB)	/* offset of first randomly-indexed entry */

/*
 * Address spaces
 */
#define	UZERO	KUSEG			/* base of user address space */
#define	UTZERO	(UZERO+MAXBY2PG)	/* 1st user text address; see mkfile */
#define	USTKTOP	(KZERO-BY2PG)		/* byte just beyond user stack */
#define	USTKSIZE (8*1024*1024)		/* size of user stack */
#define TSTKTOP (KSEG2+USTKSIZE-BY2PG)	/* top of temporary stack */
#define TSTKSIZ (1024*1024/BY2PG)	/* can be at most UTSKSIZE/BY2PG */
#define	KZERO	KSEG0			/* base of kernel address space */
#define	KTZERO	(KZERO+0x20000)		/* first address in kernel text */
#define MEMSIZE	(256*MB)		/* fixed memory on routerboard */
#define PCIMEM	0x10000000		/* on rb450g */
