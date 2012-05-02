/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#define GiB		1073741824u		/* Gibi 000000000040000000 */

#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))	/* ceiling */
#define ROUNDDN(x, y)	(((x)/(y))*(y))		/* floor */
#define MIN(a, b)	((a) < (b)? (a): (b))
#define MAX(a, b)	((a) > (b)? (a): (b))

/*
 * Not sure where these macros should go.
 * This probably isn't right but will do for now.
 * The macro names are problematic too.
 */
/*
 * In B(o), 'o' is the bit offset in the register.
 * For multi-bit fields use F(v, o, w) where 'v' is the value
 * of the bit-field of width 'w' with LSb at bit offset 'o'.
 */
#define B(o)		(1<<(o))
#define F(v, o, w)	(((v) & ((1<<(w))-1))<<(o))

#define FCLR(d, o, w)	((d) & ~(((1<<(w))-1)<<(o)))
#define FEXT(d, o, w)	(((d)>>(o)) & ((1<<(w))-1))
#define FINS(d, o, w, v) (FCLR((d), (o), (w))|F((v), (o), (w)))
#define FSET(d, o, w)	((d)|(((1<<(w))-1)<<(o)))

#define FMASK(o, w)	(((1<<(w))-1)<<(o))

/*
 * Sizes
 */
#define	BY2PG		(4*KiB)			/* bytes per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define	PGROUND(s)	ROUNDUP(s, BY2PG)
#define	ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))

/* max # of cpus system can run.  tegra2 cpu ids are two bits wide. */
#define	MAXMACH		4
#define	MACHSIZE	BY2PG
#define L1SIZE		(4 * BY2PG)

#define KSTKSIZE	(16*KiB)		/* was 8K */
#define STACKALIGN(sp)	((sp) & ~7)		/* bug: assure with alloc */

/*
 * Magic registers
 */

#define	USER		9		/* R9 is up-> */
#define	MACH		10		/* R10 is m-> */

/*
 * Address spaces.
 * KTZERO is used by kprof and dumpstack (if any).
 *
 * KZERO (0xc0000000) is mapped to physical 0 (start of dram).
 * u-boot claims to occupy the first 4 MB of dram, but we're willing to
 * step on it once we're loaded.
 *
 * L2 PTEs are stored in 4K before cpu0's Mach (8K to 12K above KZERO).
 * cpu0's Mach struct is at L1 - MACHSIZE(4K) to L1 (12K to 16K above KZERO).
 * L1 PTEs are stored from L1 to L1+32K (16K to 48K above KZERO).
 * plan9.ini is loaded at CONFADDR (4MB).
 * KTZERO may be anywhere after that.
 */
#define	KSEG0		0xC0000000		/* kernel segment */
/* mask to check segment; good for 1GB dram */
#define	KSEGM		0xC0000000
#define	KZERO		KSEG0			/* kernel address space */
#define L1		(KZERO+16*KiB)		/* cpu0 l1 page table; 16KiB aligned */
#define CONFADDR	(KZERO+0x400000)	/* unparsed plan9.ini */
#define CACHECONF	(CONFADDR+48*KiB)
/* KTZERO must match loadaddr in mkfile */
#define	KTZERO		(KZERO+0x410000)	/* kernel text start */

#define	L2pages		(2*MiB)	/* high memory reserved for l2 page tables */
#define RESRVDHIMEM	(64*KiB + MiB + L2pages) /* avoid HVECTOR, l2 pages */
/* we assume that we have 1 GB of ram, which is true for all trimslices. */
#define DRAMSIZE	GiB

#define	UZERO		0			/* user segment */
#define	UTZERO		(UZERO+BY2PG)		/* user text start */
#define UTROUND(t)	ROUNDUP((t), BY2PG)
/*
 * moved USTKTOP down to 1GB to keep MMIO space out of user space.
 * moved it down another MB to utterly avoid KADDR(stack_base) mapping
 * to high exception vectors.  see confinit().
 */
#define	USTKTOP		(0x40000000 - 64*KiB - MiB) /* user segment end +1 */
#define	USTKSIZE	(8*1024*1024)		/* user stack size */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* sysexec temporary stack */
#define	TSTKSIZ	 	256

/* address at which to copy and execute rebootcode */
#define	REBOOTADDR	KADDR(0x100)

/*
 * Legacy...
 */
#define BLOCKALIGN	CACHELINESZ		/* only used in allocb.c */
#define KSTACK		KSTKSIZE

/*
 * Sizes
 */
#define BI2BY		8			/* bits per byte */
#define BY2SE		4
#define BY2WD		4
#define BY2V		8			/* only used in xalloc.c */

#define CACHELINESZ	32			/* bytes per cache line */
#define	PTEMAPMEM	(1024*1024)
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	1984			/* magic 16*124 */
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

/*
 * Physical machine information from here on.
 */

#define PHYSDRAM	0

#define PHYSIO		0x50000000	/* cpu */
#define VIRTIO		PHYSIO
#define PHYSL2BAG	0x50043000	/* l2 cache bag-on-the-side */
#define PHYSEVP		0x6000f100	/* undocumented `exception vector' */
#define PHYSCONS	0x70006000	/* uart console */
#define PHYSIOEND	0xc0000000	/* end of ahb mem & pcie */

#define PHYSAHB		0xc0000000	/* ahb bus */
#define VIRTAHB		0xb0000000
#define P2VAHB(pa) ((pa) - PHYSAHB + VIRTAHB)

#define PHYSNOR		0xd0000000
#define VIRTNOR		0x40000000
