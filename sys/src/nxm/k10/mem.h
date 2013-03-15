/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#define GiB		1073741824u		/* Gibi 000000000040000000 */
#define TiB		1099511627776ull	/* Tebi 0x0000010000000000 */
#define PiB		1125899906842624ull	/* Pebi 0x0004000000000000 */
#define EiB		1152921504606846976ull	/* Exbi 0x1000000000000000 */

#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))
#define ROUNDDN(x, y)	(((x)/(y))*(y))
#define MIN(a, b)	((a) < (b)? (a): (b))
#define MAX(a, b)	((a) > (b)? (a): (b))

#define ALIGNED(p, a)	(!(((uintptr)(p)) & ((a)-1)))

/*
 * Sizes
 */
#define BI2BY		8			/* bits per byte */
#define BY2V		8			/* bytes per double word */
#define BY2SE		8			/* bytes per stack element */
#define BLOCKALIGN	8

/*
 * 4K pages
 * these defines could go.
 */
#define PGSZ		(4*KiB)			/* page size */
#define PGSHFT		12			/* log(PGSZ) */
#define PTSZ		(4*KiB)			/* page table page size */
#define PTSHFT		9			/*  */

#define MACHSZ		(4*KiB)			/* Mach+stack size */
#define MACHMAX		32			/* max. number of cpus */
#define MACHSTKSZ	(6*(4*KiB))		/* Mach stack size */

#define KSTACK		(16*1024)		/* Size of Proc kernel stack */
#define STACKALIGN(sp)	((sp) & ~(BY2SE-1))	/* bug: assure with alloc */

/*
 * 2M pages
 * these defines must go.
 */
#define	BIGPGSHFT	21
#define	BIGPGSZ		(1ull<<BIGPGSHFT)
#define	BIGPGROUND(x)	ROUNDUP((x), BIGPGSZ)
#define	PGSPERBIG	(BIGPGSZ/PGSZ)

/*
 * 1G pages
 */
#define	GSHIFT		30
#define	GPGSZ		(1ull<<GSHIFT)
#define	GPGROUND(x)	ROUNDUP((x),GPGSZ)
#define	PGSPERG		(GPGSZ/PGSZ)

/*
 * 512G pages
 * we don't really want most people using these.
 */
#define	_512GSHIFT	39
#define	_512GPGSZ	(1ull<<_512GSHIFT)
#define	_512GPGROUND(x)	ROUNDUP((x),_512GPGSZ)
#define	_512PGSPERG	(_512GPGSZ/PGSZ)

/*
 * Time
 */
#define HZ		(100)			/* clock frequency */
#define MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 *  Address spaces. User:
 */
#define UTZERO		(0+2*MiB)		/* first address in user text */
#define UTROUND(t)	ROUNDUP((t), BIGPGSZ)
#define USTKTOP		(0x00007ffffffff000ull & ~(2*BIGPGSZ-1))
#define USTKTLS		(USTKTOP - (2*sizeof(uintptr) + sizeof(uvlong))) /* see: tos.h. */
#define KMBASE		(0x0008000000000000ull)
#define USTKSIZE		(16*1024*1024)		/* size of user stack */
#define TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */
#define BIGBSSTOP	((TSTKTOP-BIGPGSZ) & ~(1ULL*GiB-1))
#define BIGBSSSIZE	(32ull*GiB)			/* size of big heap segment */
#define HEAPTOP		(BIGBSSTOP-BIGBSSSIZE)	/* end of shared heap segments */

/* until now, on linux emulation, we had allowed mmap and brk areas
 * to intermix. The glibc runtime can't take it. So we have to find
 * a separate place for mmap. For now, we put it at the base of the
 * USTK "_512G" page
 */
#define MMAPBASE	(_512GPGROUND(USTKTOP-_512GPGSZ))

/*
 *  Address spaces. Kernel, sorted by address.
 */
#define KSEG2		(0xfffffe0000000000ull)	/* 1TB - KMAP */
/*			 0xffffff0000000000ull	end of KSEG2 */
#define VMAP		(0xffffffffe0000000ull)
#define VMAPSZ		(256*MiB)
#define KSEG0		(0xfffffffff0000000ull)	/* 256MB - this is confused */
#define KZERO		(0xfffffffff0000000ull)
#define KTZERO		(KZERO+1*MiB+64*KiB)
#define PDMAP		(0xffffffffff800000ull)
#define PMAPADDR		(0xffffffffffe00000ull)	/* unused as of yet */
/*			 0xffffffffffffffffull	end of KSEG0 */



/*
 * Hierarchical Page Tables.
 * For example, traditional IA-32 paging structures have 2 levels,
 * level 1 is the PD, and level 0 the PT pages; with IA-32e paging,
 * level 3 is the PML4(!), level 2 the PDP, level 1 the PD,
 * and level 0 the PT pages. The PTLX macro gives an index into the
 * page-table page at level 'l' for the virtual address 'v'.
 */
#define PTLX(v, l)	(((v)>>(((l)*PTSHFT)+PGSHFT)) & ((1<<PTSHFT)-1))
#define PGLSZ(l)	(1<<(((l)*PTSHFT)+PGSHFT))

