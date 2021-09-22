/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
/* from ../port/portdat.h */
#ifndef MB
#define KB		1024
#define MB		(1024*1024)
#define GB		(1024*MB)
#endif

/*
 * Not sure where these macros should go.
 * This probably isn't right but will do for now.
 * The macro names are problematic too.
 */
/*
 * In BITN(o), 'o' is the bit offset in the register.
 * For multi-bit fields use F(v, o, w) where 'v' is the value
 * of the bit-field of width 'w' with LSb at bit offset 'o'.
 */
#define BITN(o)		(1<<(o))
#define F(v, o, w)	(((v) & ((1<<(w))-1))<<(o))

/*
 * Sizes
 */
#define	BY2PG		(4*KB)			/* bytes per page */
#define	PGSHIFT		12			/* log(BY2PG) */

/* max # of cpus system can run.  tegra2 cpu ids are two bits wide. */
#define	MAXMACH		4
#define	MACHSIZE	BY2PG
#define L1SIZE		(16*KB)

#define KSTKSIZE	(8*KB)			/* typical use is 2580 bytes */
#define STACKALIGN(sp)	((sp) & ~7)		/* bug: assure with alloc */

/*
 * Magic registers
 */

#define	USER		9		/* R9 is up-> */
#define	MACH		10		/* R10 is m-> */

/*
 * Address spaces.
 *
 * Low memory.
 *
 * KTZERO is used by kprof and dumpstack (if any).
 * KZERO (0xc0000000) is mapped to physical 0 (start of dram).
 * u-boot claims to occupy the first 4 MB of dram (up to 0x408000),
 * but we're willing to step on it once we're loaded.
 *
 * 0		vectors
 * 64		wfiloop
 * 0x100	reboot code
 * 4k		cpu0 mach
 * 16k		cpu0 l1 ptes, 16k-aligned
 * 32k		cpu1 l1 ptes, 16k-aligned
 * 48k		cpu2 l1 ptes, 16k-aligned
 * 64k		cpu3 l1 ptes, 16k-aligned
 * 80k		cacheconf
 * ...
 * uboot
 * ...
 * 0x408000	uboot's loadaddr (4MB + 32K)
 * 0x410000	confaddr: uboot loads /cfg/pxe/$mac file here (4MB + 64K)
 * 0x420000	KTZERO: kernel start (4MB + 128K)
 */
/* virtual addresses */
#define	KSEG0		0xC0000000		/* kernel segment */
/* mask to check segment; good for 1GB dram */
#define	KSEGM		0xC0000000
#define	KZERO		KSEG0			/* kernel address space */

/* address at which to copy and execute rebootcode (under 3KB now) */
#define	REBOOTADDR	(KZERO+0x100)
#define LOWZSTART	MACHADDR		/* l.s starts zeroing here */
#define MACHADDR	(KZERO+4*KB)		/* only cpu0's */
#define L1CPU0		(KZERO+L1SIZE)	/* cpu0 l1 page table; 16KB aligned */
#define L1CPU1		(L1CPU0+L1SIZE)	/* cpu1 l1 page table; 16KB aligned */
#define LOWZEND		L1CPU1			/* l.s stops zeroing before */
#define CACHECONF	(L1CPU0 + MAXMACH*L1SIZE)

#define CONFADDR	(KZERO+0x410000) /* unparsed plan9.ini from uboot */
/* KTZERO must match loadaddr in mkfile */
#define	KTZERO		(CONFADDR+0x10000)	/* kernel text start */

#define MAXMB	(KB-1)		/* last MB has vectors */
#define DOUBLEMAPMBS MAXMB	/* megabytes of low dram to double-map */
#define ARGLINK	(2*BY2WD)	/* stack space for one argument and link */

/*
 * High memory.
 *
 * top of l.s initial temporary stack
 * l2 page pool
 * gap to high vectors (1MB)
 * high vectors (64K at 0xffff0000)
 */

/* physical addresses, and sizes */
/* avoid HVECTORS utterly & the l2 pool; see mmu.c */
#define RESRVDHIMEM	(L2POOLSIZE + HIVECTGAP)
#define L2POOLBASE	(PHYSDRAM + DRAMSIZE - RESRVDHIMEM)	/* phys base */
#define	L2POOLSIZE	(2*MB)		/* high memory for l2 page tables */
#define HIVECTGAP	(MB + HIVECTSIZE)
#define HVECTORS	0xffff0000
#define HIVECTSIZE	(64*KB)
#define TMPSTACK	(L2POOLBASE-BY2WD) /* phys top, only during startup */
/* we assume that we have 1 GB of ram, which is true for all trimslices. */
#define DRAMSIZE	GB

/* virtual addresses */
#define	UZERO		0			/* user segment */
#define	UTZERO		(UZERO+BY2PG)		/* user text start */
#define UTROUND(t)	ROUNDUP((t), BY2PG)
/*
 * moved USTKTOP down to 1GB to keep MMIO space out of user space.
 * moved it down another ~MB to utterly avoid KADDR(stack_base) mapping
 * to high exception vectors.
 */
#define	USTKTOP		(0x40000000 - HIVECTGAP) /* user segment end +1 */
#define	USTKSIZE	(8*MB)			/* max. user stack size */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* sysexec temporary stack */
#define	TSTKSIZ	 	256

/*
 * Legacy...
 */
#define BLOCKALIGN	CACHELINESZ		/* only used in allocb.c */
#define KSTACK		KSTKSIZE

/*
 * Sizes
 */
#define BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define BY2SE		4
#define BY2WD		4
#define BY2V		8			/* only used in xalloc.c */

#define CACHELINESZ	32			/* bytes per cache line */
#define	PTEMAPMEM	(1024*1024)
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	(ROUND(USTKTOP, PTEMAPMEM) / PTEMAPMEM)
#define	SSEGMAPSIZE	16			/* magic */
#define	PPN(x)		((x)&~(BY2PG-1))	/* pure page number? */

/*
 * With a little work these move to port.
 */
#define	PTEVALID	(1<<0)
#define	PTERONLY	0
#define	PTEWRITE	(1<<1)
#define	PTEUNCACHED	(1<<2)
#define PTEKERNEL	(1<<3)

/* L1 arm pte values */
#define PTEDRAM	(Dom0|L1AP(Krw)|Section|L1ptedramattrs)
#define PTEIO	(Dom0|L1AP(Krw)|Section|Noexecsect|L1sharable)

/*
 * Physical machine information from here on.
 */

#define PHYSDRAM	0

#define PHYSIO		0x50000000	/* cpu */
#define VIRTIO		PHYSIO
#define PHYSSCU		0x50040000	/* scu, also periphbase: private */
#define PHYSL2BAG	0x50043000	/* l2 cache bag-on-the-side */
#define PHYSCLKRST	0x60006000	/* clocks & resets */
#define PHYSEVP		0x6000f100	/* undocumented `exception vector' */
#define PHYSCONS	0x70006000	/* uart console */
#define PHYSIOEND	0xc0000000	/* end of ahb mem & pcie */

#define PHYSAHB		0xc0000000	/* ahb bus */
#define VIRTAHB		0xb0000000
#define P2VAHB(pa) ((pa) - PHYSAHB + VIRTAHB)

#define PHYSNOR		0xd0000000
#define VIRTNOR		0x40000000
