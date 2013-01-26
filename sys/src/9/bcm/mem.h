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
 * Sizes
 */
#define	BY2PG		(4*KiB)			/* bytes per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define	PGROUND(s)	ROUNDUP(s, BY2PG)
#define	ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))

#define	MAXMACH		1			/* max # cpus system can run */
#define	MACHSIZE	BY2PG

#define KSTKSIZE	(8*KiB)
#define STACKALIGN(sp)	((sp) & ~3)		/* bug: assure with alloc */

/*
 * Address spaces.
 * KTZERO is used by kprof and dumpstack (if any).
 *
 * KZERO is mapped to physical 0 (start of ram).
 *
 * vectors are at 0, plan9.ini is at KZERO+256 and is limited to 16K by
 * devenv.
 */

#define	KSEG0		0x80000000		/* kernel segment */
/* mask to check segment; good for 512MB dram */
#define	KSEGM		0xE0000000
#define	KZERO		KSEG0			/* kernel address space */
#define CONFADDR	(KZERO+0x100)		/* unparsed plan9.ini */
#define	MACHADDR	(KZERO+0x2000)		/* Mach structure */
#define	L2		(KZERO+0x3000)		/* L2 ptes for vectors etc */
#define	VCBUFFER	(KZERO+0x3400)		/* videocore mailbox buffer */
#define	FIQSTKTOP	(KZERO+0x4000)		/* FIQ stack */
#define	L1		(KZERO+0x4000)		/* tt ptes: 16KiB aligned */
#define	KTZERO		(KZERO+0x8000)		/* kernel text start */
#define VIRTIO		0x7E000000		/* i/o registers */
#define	FRAMEBUFFER	0xA0000000		/* video framebuffer */

#define	UZERO		0			/* user segment */
#define	UTZERO		(UZERO+BY2PG)		/* user text start */
#define	USTKTOP		0x20000000		/* user segment end +1 */
#define	USTKSIZE	(8*1024*1024)		/* user stack size */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* sysexec temporary stack */
#define	TSTKSIZ	 	256

/* address at which to copy and execute rebootcode */
#define	REBOOTADDR	(KZERO+0x3400)

/*
 * Legacy...
 */
#define BLOCKALIGN	32			/* only used in allocb.c */
#define KSTACK		KSTKSIZE

/*
 * Sizes
 */
#define BI2BY		8			/* bits per byte */
#define BY2SE		4
#define BY2WD		4
#define BY2V		8			/* only used in xalloc.c */

#define CACHELINESZ	32
#define	PTEMAPMEM	(1024*1024)
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	1984
#define	SSEGMAPSIZE	16
#define	PPN(x)		((x)&~(BY2PG-1))

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
 *	PHYS addresses as seen from the arm cpu.
 *	BUS  addresses as seen from the videocore gpu.
 */
#define	PHYSDRAM	0
#define BUSDRAM		0x40000000
#define	DRAMSIZE	(512*MiB)
#define	PHYSIO		0x20000000
#define	BUSIO		0x7E000000
#define	IOSIZE		(16*MiB)
