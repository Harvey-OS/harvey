/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define 	BY2V		8			/* bytes per vlong */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define ROUND(s, sz)	(((s)+((sz)-1))&~((sz)-1))
#define 	PGROUND(s)	ROUND(s, BY2PG)

#define	MAXMACH		1			/* max # cpus system can run */
#define	KSTACK		4096			/* Size of kernel stack */
#define BLOCKALIGN	8

/*
 * Time
 */
#define	HZ		100			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((t)*MS2HZ)		/* ticks to milliseconds */
#define	MS2TK(t)	(((t)*HZ+500)/1000)	/* milliseconds to closest tick */

/*
 * CP0 registers
 */

#define INDEX		0
#define RANDOM		1
#define TLBPHYS0	2
#define TLBPHYS1	3
#define CONTEXT		4
#define PAGEMASK	5
#define WIRED		6
#define BADVADDR	8
#define COUNT		9
#define TLBVIRT		10
#define COMPARE		11
#define STATUS		12
#define CAUSE		13
#define EPC		14
#define	PRID		15
#define	CONFIG		16
#define	LLADDR		17
#define	WATCHLO		18
#define	WATCHHI		19
#define	CACHEECC	26
#define	CACHEERR	27
#define	TAGLO		28
#define	TAGHI		29
#define	ERROREPC	30

/*
 * M(STATUS) bits
 */
#define KMODEMASK	0x0000001f
#define IE		0x00000001
#define EXL		0x00000002
#define ERL		0x00000004
#define KSUPER		0x00000008
#define KUSER		0x00000010
#define KSU		0x00000018
#define UX		0x00000020
#define SX		0x00000040
#define KX		0x00000080
#define INTMASK		0x0000ff00
#define INTR0		0x00000100
#define INTR1		0x00000200
#define INTR2		0x00000400
#define INTR3		0x00000800
#define INTR4		0x00001000
#define INTR5		0x00002000
#define INTR6		0x00004000
#define INTR7		0x00008000
#define DE		0x00010000
#define BEV		0x00400000
#define CU0		0x10000000
#define CU1		0x20000000

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
#define	MACH	25		/* R25 is m-> */

/*
 * Fundamental addresses
 */
#define	MACHADDR	(KTZERO-MAXMACH*BY2PG)	/* warning: rdbg is near here */
#define	MACHP(n)	((Mach *)(MACHADDR+(n)*BY2PG))

/*
 * offsets in ureg.h for l.s
 */
#define	Ureg_status	(Uoffset+0)
#define	Ureg_pc		(Uoffset+4)
#define	Ureg_sp		(Uoffset+8)
#define	Ureg_cause	(Uoffset+12)
#define	Ureg_badvaddr	(Uoffset+16)
#define	Ureg_tlbvirt	(Uoffset+20)

#define	Ureg_hi		(Uoffset+28)
#define	Ureg_lo		(Uoffset+36)
#define	Ureg_r31	(Uoffset+44)
#define	Ureg_r30	(Uoffset+52)
#define	Ureg_r28	(Uoffset+60)
#define	Ureg_r27	(Uoffset+68)
#define	Ureg_r26	(Uoffset+76)
#define	Ureg_r25	(Uoffset+84)
#define	Ureg_r24	(Uoffset+92)
#define	Ureg_r23	(Uoffset+100)
#define	Ureg_r22	(Uoffset+108)
#define	Ureg_r21	(Uoffset+116)
#define	Ureg_r20	(Uoffset+124)
#define	Ureg_r19	(Uoffset+132)
#define	Ureg_r18	(Uoffset+140)
#define	Ureg_r17	(Uoffset+148)
#define	Ureg_r16	(Uoffset+156)
#define	Ureg_r15	(Uoffset+164)
#define	Ureg_r14	(Uoffset+172)
#define	Ureg_r13	(Uoffset+180)
#define	Ureg_r12	(Uoffset+188)
#define	Ureg_r11	(Uoffset+196)
#define	Ureg_r10	(Uoffset+204)
#define	Ureg_r9		(Uoffset+212)
#define	Ureg_r8		(Uoffset+220)
#define	Ureg_r7		(Uoffset+228)
#define	Ureg_r6		(Uoffset+236)
#define	Ureg_r5		(Uoffset+244)
#define	Ureg_r4		(Uoffset+252)
#define	Ureg_r3		(Uoffset+260)
#define	Ureg_r2		(Uoffset+268)
#define	Ureg_r1		(Uoffset+276)

/* Sizeof(Ureg) + (R5,R6) + 16 bytes slop + retpc + ur */
#define	UREGSIZE	((Ureg_r1+4-Uoffset) + 16 + 16 + 4 + 4)
#define	Uoffset		8

/*
 * MMU
 */
#define	PGSZ4K		(0x00<<13)
#define	PGSZ64K		(0x0F<<13)
#define	PGSZ256K	(0x3F<<13)
#define	PGSZ1M		(0xFF<<13)
#define	PGSZ4M		(0x3FF<<13)

#define	KUSEG	0x00000000
#define	KSEG0	0x80000000
#define	KSEG1	0xA0000000
#define	KSEG2	0xC0000000
#define	KSEG3	0xE0000000
#define	KSEGM	0xE0000000	/* mask to check which seg */
#define	CACHELINESZ	128

#define	PIDXSHFT	12
#define	PIDX		(0x7<<PIDXSHFT)
#define	KMAPADDR	0xE4000000
#define	KMAPMASK	0xFF000000
#define	KMAPSHIFT	15
#define	NCOLOR		8
#define	getpgcolor(a)	(((ulong)(a)>>PIDXSHFT)&7)

#define	PTEGLOBL	(1<<0)
#define	PTEVALID	(1<<1)
#define	PTEWRITE	(1<<2)
#define	PTERONLY	0
#define	PTEALGMASK	(7<<3)
#define	PTEUNCACHED	(2<<3)
#define	PTENONCOHER	(3<<3)
#define	PTECOHERXCL	(4<<3)
#define	PTECOHERXCLW	(5<<3)
#define	PTECOHERUPDW	(6<<3)
#define	IOPTE		(PTEGLOBL|PTEVALID|PTEWRITE|PTEUNCACHED)

#define	PTEPID(n)	(n)
#define	TLBPID(n)	((n)&0xFF)
#define	PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	STLBLOG		13
#define	STLBSIZE	(1<<STLBLOG)
#define	KPTELOG		7
#define	KPTESIZE	(1<<KPTELOG)
#define	SEGMAPSIZE	512
#define SSEGMAPSIZE	16

#define	NTLBPID	256	/* number of pids */
#define	NTLB	48	/* number of entries */
#define	TLBROFF	5	/* offset of first randomly indexed entry */

/*
 * Address spaces
 */

#define	UZERO	KUSEG			/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	USTKTOP	(KZERO-BY2PG)		/* byte just beyond user stack */
#define	TSTKTOP	(0xC0000000+USTKSIZE-BY2PG) /* top of temporary stack */
#define	TSTKSIZ 100
#define	KZERO	KSEG0			/* base of kernel address space */
#define	KTZERO	(KZERO+0x12000)		/* first address in kernel text */
#define	USTKSIZE	(8*1024*1024)	/* size of user stack */
#define globalmem(x)	(((ulong)x)&KZERO)	/* addresses valid in all contexts */
/*
 * Exception codes
 */
#define	EXCMASK	0x1f		/* mask of all causes */
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
#define	CTRAP	13		/* trap */
#define	CVCEI	14		/* virtual coherence exception (instruction) */
#define	CFPE	15		/* floating point exception */
#define	CWATCH	23		/* watch exception */
#define	CVCED	31		/* virtual coherence exception (data) */

#define isphys(x) (((ulong)x&KSEGM)==KSEG0 || ((ulong)x&KSEGM)==KSEG1)
