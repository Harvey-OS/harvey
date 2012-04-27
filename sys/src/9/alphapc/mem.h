/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define BY2V		8			/* bytes per vlong */
#define	BY2PG		8192			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		13			/* log(BY2PG) */
#define ROUND(s, sz)	(((s)+((sz)-1))&~((sz)-1))
#define PGROUND(s)	ROUND(s, BY2PG)
#define BLOCKALIGN	8

#define	BY2PTE		8			/* bytes per pte entry */
#define	PTE2PG		(BY2PG/BY2PTE)		/* pte entries per page */

#define	MAXMACH		1			/* max # cpus system can run */
#define	KSTACK		4096			/* Size of kernel stack */

/*
 * Time
 */
#define	HZ		100			/* clock frequency */
#define	MS2HZ		(1000/HZ)
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 * Magic registers
 */
#define	MACH		15		/* R15 is m-> */
#define	USER		14		/* R14 is up-> */


/*
 * Fundamental addresses
 */
/* XXX MACHADDR, MACHP(n) */

/*
 * MMU
 *
 * A PTE is 64 bits, but a ulong is 32!  Hence we encode
 * the PTEs specially for fault.c, and decode them in putmmu().
 * This means that we can only map the first 2G of physical
 * space via putmmu() - ie only physical memory, not devices.
 */
#define	PTEVALID	0x3301
#define	PTEKVALID	0x1101
#define	PTEASM		0x0010
#define	PTEGH(s)	((s)<<5)
#define	PTEWRITE	0
#define	PTERONLY	0x4
#define	PTEUNCACHED	0
#define	PPN(n)		(((n)>>PGSHIFT)<<14)
#define	FIXPTE(x)	((((uvlong)(x)>>14)<<32)|((x) & 0x3fff))
#define	PTEPFN(pa)	(((uvlong)(pa)>>PGSHIFT)<<32)
#define	NCOLOR		1
#define	getpgcolor(a)	0

#define	PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	512
#define SSEGMAPSIZE	16

/*
 * Address spaces
 */
#define	UZERO	0			/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	USTKTOP	(TSTKTOP-TSTKSIZ*BY2PG)	/* byte just beyond user stack */
#define	TSTKTOP	KZERO			/* top of temporary stack */
#define	TSTKSIZ 100
#define	KZERO	0x80000000		/* base of kernel address space */
#define	KTZERO	(KZERO+0x400000)	/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

/*
 * Processor Status (as returned by rdps)
 */
#define	UMODE	0x8
#define	IPL	0x7


#define isphys(x) (((ulong)x&KZERO)!=0)
