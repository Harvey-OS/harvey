/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2V		8			/* bytes per double word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	BY2XPG		(4096*1024)		/* bytes per big page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define CACHELINESZ	32			/* pentium & later */
#define	BLOCKALIGN	8

#define	MAXMACH		1			/* max # cpus system can run */
/*
 * we use a larger-than-normal kernel stack in the bootstraps as we have
 * significant arrays (buffers) on the stack.  we typically consume about
 * 4.5K of stack.
 */
#define KSTACK		(4*BY2PG)		/* Size of kernel stack */
#define MACHSIZE	BY2PG

/*
 * Time
 */
#define	HZ		(100)			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 *  Address spaces
 *
 *  Kernel is at 2GB-4GB
 */
#define	KZERO		0x80000000		/* base of kernel address space */
#define	KSEGM		0xF0000000
#define	VPT		(KZERO-VPTSIZE)
#define	VPTSIZE		BY2XPG
#define	NVPT		(VPTSIZE/BY2WD)
#define	KMAP		(VPT-KMAPSIZE)
#define	KMAPSIZE	BY2XPG
#define	VMAP		(KMAP-VMAPSIZE)
#define	VMAPSIZE	(0x10000000-VPTSIZE-KMAPSIZE)
#define	UZERO		0			/* base of user address space */
#define	UTZERO		(UZERO+BY2PG)		/* first address in user text */
#define UTROUND(t)	ROUNDUP((t), BY2PG)
#define	USTKTOP		(VMAP-BY2PG)		/* byte just beyond user stack */
#define	USTKSIZE	(16*1024*1024)		/* size of user stack */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */
#define	TSTKSIZ 	100			/* pages in new stack; limits exec args */

/*
 * Fundamental addresses - bottom 64kB saved for return to real mode
 *
 * we need some fixed address space for things that normally fit below the
 * kernel and some of it needs to be addressible from real mode, thus under 1MB.
 *
 * pxe loading starts us at 0x7c00 and non-pxe loading starts us at 0x10000.
 *
 * this assertion must hold: PDX(TMPADDR) == PDX(MACHADDR).
 * see realmode0.s for a description of the Chinese puzzle.
 */
/* first 1K is real-mode IVT (vectors) */
/* next 256 bytes are bios data area (bda) */
/* 0x500 to 0x800 unused [768] */
/*
 * RMCODE should be the lowest address used in real mode.
 * all real-mode buffers, etc. should follow it.
 */
#define RMCODE		(KZERO+0x800)	/* copy of initial KTZERO [2K, magic] */
#define	REBOOTADDR	(RMCODE-KZERO)	/* reboot code - physical address [128] */
#define RMSIZE		2048
/* 0x1000 to 0x1200 unused [512] */
/* CONFADDR must match def'n in kernel being loaded */
#define	CONFADDR	(KZERO+0x1200)	/* cfg passed to kernel [3.5K, fixed] */
#define BIOSXCHG	(KZERO+0x2000)	/* BIOS data exchange [2K+32] */
/* 0x2820 to 0x2900 unused [224] */
#define	RMUADDR		(KZERO+0x2900)	/* real mode Ureg [128 (76 actual)] */
/* 0x2980 to 0x3000 unused [1664] */
#define IDTADDR		(KZERO+0x3000)	/* protected-mode idt [2K] */
/* 0x3800 to 0x4000 unused [2K] */

/* we only use one processor for bootstrapping, so merge MACHADDR & CPU0MACH */
#define	MACHADDR	(KZERO+0x4000)	/* as seen by current processor */
#define	CPU0MACH	MACHADDR	/* Mach for bootstrap processor */
#define	CPU0GDT		(KZERO+0x5000)	/* bootstrap processor GDT after Mach */
#define	TMPADDR		(KZERO+0x6000)	/* used for temporary mappings */
/* 0x7000—0x7800 unused [2K], could be extra real-mode stack */
#define PXEBASE 	0x7c00		/* pxe loads us here */
#define RMSTACK 	PXEBASE		/* real phys. stack base [1K below] */
#define PBSBASE		0x10000		/* pbs loads us here (at 64K) */

/*
 * we used to use 9boot's `end', rounded up, but that was when the boot loader
 * was in the first 640K; now end is up around 10MB (at least for 9boot).
 * various machines nibble away at the top of the lowest 640K.
 */
#define BIOSTABLES	(512*1024)
// #define BIOSTABLES	(600*1024)	/* fails on amd64 */
// #define BIOSTABLES	0x3800		/* 2K: fails on amd64 */

#define Bootkernaddr	(9*MB)	/* where to put decompressed boot kernel */
#define Bootkernmax	(4*MB)	/* max size */
#define Unzipbuf	(13*MB)
#define Mallocbase	(16*MB)
/*
 * MemMin is what the bootstrap code in l*.s has already mapped;
 * MemMax is the limit of physical memory to scan.
 */
#define MemMin		(20*MB)	/* don't have PTEs for more allocated yet */
#define MemMax		(512*MB) /* allow for huge 10gbe buffers */

#define Lowmemsz	(640*KB)

#define LOWPTEPAGES	(MemMin / (4*MB))

#define Kernelmax	(8*MB)	/* max size of real kernel, not an address */

/*
 *  known x86 segments (in GDT) and their selectors
 */
#define	NULLSEG	0	/* null segment */
#define	KDSEG	1	/* kernel data/stack */
#define	KESEG	2	/* kernel executable */	
#define	UDSEG	3	/* user data/stack */
#define	UESEG	4	/* user executable */
#define	TSSSEG	5	/* task segment */
#define	APMCSEG		6	/* APM code segment */
#define	APMCSEG16	7	/* APM 16-bit code segment */
#define	APMDSEG		8	/* APM data segment */
#define	KESEG16		9	/* kernel executable 16-bit */
#define	NGDT		10	/* number of GDT entries required */
/* #define	APM40SEG	8	/* APM segment 0x40 */

#define	SELGDT	(0<<2)	/* selector is in gdt */
#define	SELLDT	(1<<2)	/* selector is in ldt */

#define	SELECTOR(i, t, p)	(((i)<<3) | (t) | (p))

#define	NULLSEL	SELECTOR(NULLSEG, SELGDT, 0)
#define	KDSEL	SELECTOR(KDSEG, SELGDT, 0)
#define	KESEL	SELECTOR(KESEG, SELGDT, 0)
#define	UESEL	SELECTOR(UESEG, SELGDT, 3)
#define	UDSEL	SELECTOR(UDSEG, SELGDT, 3)
#define	TSSSEL	SELECTOR(TSSSEG, SELGDT, 0)
#define	APMCSEL 	SELECTOR(APMCSEG, SELGDT, 0)
#define	APMCSEL16	SELECTOR(APMCSEG16, SELGDT, 0)
#define	APMDSEL		SELECTOR(APMDSEG, SELGDT, 0)
/* #define	APM40SEL	SELECTOR(APM40SEG, SELGDT, 0) */

/*
 *  fields in segment descriptors
 */
#define	SEGDATA	(0x10<<8)	/* data/stack segment */
#define	SEGEXEC	(0x18<<8)	/* executable segment */
#define	SEGTSS	(0x9<<8)	/* TSS segment */
#define	SEGCG	(0x0C<<8)	/* call gate */
#define	SEGIG	(0x0E<<8)	/* interrupt gate */
#define	SEGTG	(0x0F<<8)	/* trap gate */
#define	SEGTYPE	(0x1F<<8)

#define	SEGP	(1<<15)		/* segment present */
#define	SEGPL(x) ((x)<<13)	/* priority level */
#define	SEGB	(1<<22)		/* granularity 1==4k (for expand-down) */
#define	SEGG	(1<<23)		/* granularity 1==4k (for other) */
#define	SEGE	(1<<10)		/* expand down */
#define	SEGW	(1<<9)		/* writable (for data/stack) */
#define	SEGR	(1<<9)		/* readable (for code) */
#define	SEGD	(1<<22)		/* default 1==32bit (for code) */

/*
 *  virtual MMU
 */
#define	PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	1984
#define	SSEGMAPSIZE	16
#define	PPN(x)		((x)&~(BY2PG-1))

/*
 *  physical MMU
 */
#define	PTEVALID	(1<<0)
#define	PTEWT		(1<<3)
#define	PTEUNCACHED	(1<<4)
#define	PTEWRITE	(1<<1)
#define	PTERONLY	(0<<1)
#define	PTEKERNEL	(0<<2)
#define	PTEUSER		(1<<2)
#define	PTESIZE		(1<<7)
#define	PTEGLOBAL	(1<<8)

/* CR0 */
#define PG	0x80000000

/*
 * Macros for calculating offsets within the page directory base
 * and page tables. 
 */
#define	PDX(va)		((((ulong)(va))>>22) & 0x03FF)
#define	PTX(va)		((((ulong)(va))>>12) & 0x03FF)

#define	getpgcolor(a)	0

