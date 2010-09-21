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

#define	MAXMACH		1			/* max # cpus system can run */
#define	MACHSIZE	BY2PG

#define KSTKSIZE	(8*KiB)
#define STACKALIGN(sp)	((sp) & ~3)		/* bug: assure with alloc */

/*
 * Address spaces.
 * KTZERO is used by kprof and dumpstack (if any).
 *
 * KZERO is mapped to physical 0.
 * u-boot claims to take 0 - 8MB.
 *
 * vectors are at 0, plan9.ini is at KZERO+4K and is limited to 16K by
 * devenv.  L2 PTEs for trap vectors & i/o regs are stored from KZERO+56K
 * to L1-MACHSIZE (KZERO+60K).  cpu0's Mach struct is at L1 - MACHSIZE(4K)
 * to L1 (KZERO+60K to KZERO+64K).  L1 PTEs are stored from L1 to L1+32K
 * (KZERO+64K to KZERO+96K).  KTZERO may be anywhere after KZERO+96K.
 */

#define	KSEG0		0x60000000		/* kernel segment */
/* mask to check segment; good for 512MB dram */
#define	KSEGM		0xE0000000
#define	KZERO		KSEG0			/* kernel address space */
#define CONFADDR	(KZERO+4*KiB)		/* unparsed plan9.ini */
#define L1		(KZERO+64*KiB)		/* tt ptes: 16KiB aligned */
#define	KTZERO		(KZERO+0x800000)	/* kernel text start */

#define	UZERO		0			/* user segment */
#define	UTZERO		(UZERO+BY2PG)		/* user text start */
#define	USTKTOP		KZERO			/* user segment end +1 */
#define	USTKSIZE	(8*1024*1024)		/* user stack size */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* sysexec temporary stack */
#define	TSTKSIZ	 	256

/* address at which to copy and execute rebootcode */
#define	REBOOTADDR	KADDR(0x100)

/*
 * Time.
 * Does this need to be here? Used in assembler?
 */
#define	HZ		100			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 * More accurate time
 */
#define CLOCKFREQ	(200*1000*1000)		/* TCLK on sheeva: 200MHz */
//#define MS2TMR(t)	((ulong)(((uvlong)(t)*CLOCKFREQ)/1000))
//#define US2TMR(t)	((ulong)(((uvlong)(t)*CLOCKFREQ)/1000000))

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
 */
#define PHYSDRAM	0

/* from 0x80000000 up is uncached by L2 (see archkw.c) */
#define PHYSCESASRAM	0xc8010000
// #define PHYSSPIFLASH	0xe8000000		/* ignore spi flash */
/* this address is configured by u-boot, and is 0xd0000000 at reset */
#define PHYSIO		0xf1000000		/* internal registers */
#define PHYSCONS	(PHYSIO + 0x12000)	/* uart */
#define PHYSNAND1	0xf9000000		/* sheeva/openrd (remapped) */
#define PHYSNAND2	0xd8000000		/* guru */
#define PHYSBOOTROM	0xffff0000		/* boot rom */

#define FLASHSIZE	(512*MiB)		/* but not addressed linearly */

#define VIRTIO		PHYSIO
