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
#define	BY2PG		8192		/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)	/* words per page */
#define	PGSHIFT		13			/* log(BY2PG) */
#define 	ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))
#define 	PGROUND(s)	ROUND(s, BY2PG)
#define BLOCKALIGN	8

#define	BY2PTE		8			/* bytes per pte entry */
#define	PTE2PG		(BY2PG/BY2PTE)	/* pte entries per page */

#define	MAXMACH		1			/* max # cpus system can run */
#define	KSTACK		4096			/* Size of kernel stack */

/*
 * Time
 */
#define	HZ		100			/* clock frequency */
#define	MS2HZ	(1000/HZ)
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 * Magic registers
 */

#define	MACH	15		/* R15 is m-> */
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

#define	PTEVALID		0x3301
#define	PTEKVALID	0x1101
#define	PTEASM		0x0010
#define	PTEGH(s)		((s)<<5)
#define	PTEWRITE		0
#define	PTERONLY	0x4
#define	PTEUNCACHED	0
#define	PPN(n)		(((n)>>PGSHIFT)<<14)
#define	FIXPTE(x)		((((uvlong)(x)>>14)<<32)|((x) & 0x3fff))
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
#define	TSTKTOP	KZERO	/* top of temporary stack */
#define	TSTKSIZ 100
#define	KZERO	0x80000000	/* base of kernel address space */
#define	KTZERO	(KZERO+0x400000)		/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

/*
 * Palcode instructions a la OSF/1
 */
#define	PALbpt		0x80
#define	PALbugchk	0x81
#define	PALcallsys	0x83
#define	PALimb		0x86
#define	PALgentrap	0xaa
#define	PALrdunique	0x9e
#define	PALwrunique	0x9f

#define	PALhalt		0x00
#define	PALdraina	0x02
#define	PALcserve	0x09
#define	PALrdps		0x36
#define	PALrdusp	0x3a
#define	PALrdval	0x32
#define	PALretsys	0x3d
#define	PALrti		0x3f
#define	PALswpctx	0x30
#define	PALswpipl	0x35
#define	PALtbi		0x33
#define	PALwhami	0x3c
#define	PALwrent	0x34
#define	PALwrfen	0x2b
#define	PALwrkgp	0x37
#define	PALwrusp	0x38
#define	PALwrval	0x31
#define	PALwrvptptr	0x2d

/*
 * Plus some useful VMS ones (needed at early boot time)
 */
#define	PALmfpr_pcbb	0x12
#define	PALmfpr_ptbr	0x15
#define	PALmfpr_vptb	0x29
#define	PALldqp		0x03
#define	PALstqp		0x04
#define	PALswppal	0x0a

/*
 * Processor Status (as returned by rdps)
 */
#define	UMODE	0x8
#define	IPL		0x7


#define isphys(x) (((ulong)x&KZERO)!=0)
