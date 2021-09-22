/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

#define TOS(stktop)	((stktop)-sizeof(Tos))

/*
 * parameters & devices needed early during boot.
 */
#define PAClint		0x02000000 /* appears to be common, if not standard. */
#ifndef PAUart0
#define PAUart0		0x10010000	/* sifive's default */
#endif

#ifdef TINYEMU
#define MACHMAX		1
#define MACHINITIAL	1		/* flag: initial mode is machine */
/*
 * tinyemu is less busy & much more responsive without WFI, but it burns
 * power.  Using WFI makes even console echoing take seconds.
 */
#define IDLE_WFI	WFI	/* PAUSE */
#define	SV48
/* tinyemu only implements 32-bit clint timer access */
#define RDCLTIME	rdcltime
#define WRCLTIME	wrcltime
#define RDCLTIMECMP	rdcltimecmp
#define WRCLTIMECMP	wrcltimecmp

#else				/* TINYEMU */

/*
 * max. # of cpus (was 32); costs 1 byte/cpu in sizeof(Page)
 */
#define MACHMAX		16
#define MACHINITIAL	0		/* initial mode is super */
#define IDLE_WFI	WFI /* will be awakened by an intr, even if disabled */
#define RDCLTIME(clint)		(clint)->mtime
#define WRCLTIME(clint, v)	(clint)->mtime = (v)
#define RDCLTIMECMP(clint)	(clint)->mtimecmp[m->hartid]
#define WRCLTIMECMP(clint, v)	(clint)->mtimecmp[m->hartid] = (v)
#endif				/* TINYEMU */

#define HARTMAX		(MACHMAX + 2)	/* +2 is a guess at slop */

/* "a" must be power of 2 */
#define ALIGNED(p, a)		(((uintptr)(p) & ((a)-1)) == 0)
#define PADDRINPTE(pte)		(((vlong)(pte) >> PTESHFT) * PGSZ)
#define PAGEPRESENT(pte)	((pte) & PteP)

#define KB		1024			/* 2^10 0x0000000000000400 */
#define MB		1048576			/* 2^20 0x0000000000100000 */
#define GB		1073741824		/* 2^30 000000000040000000 */
#define TB		1099511627776ll		/* 2^40 0x0000010000000000 */
#define PB		1125899906842624ll	/* 2^50 0x0004000000000000 */
#define EB		1152921504606846976ll	/* 2^60 0x1000000000000000 */

/* old names */
#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))
#define ROUNDDN(x, y)	(((x)/(y))*(y))

#define MIN(a, b)	((a) < (b)? (a): (b))
#define MAX(a, b)	((a) > (b)? (a): (b))

/*
 * Sizes
 */
#define BI2BY		8		/* bits per byte */
#define BY2V		8		/* bytes per double word */
/* a stack element is the minimum width that the compiler will push. */
#define BY2SE		8		/* bytes per stack element (uintptr) */
#define BLOCKALIGN	8
#define FPalign		16		/* required for FXSAVE */
#define CACHELINESZ	64
#define SBIALIGN	16		/* stack alignment for SBI calls */
/* old names */
#define BY2WD		4
#define BI2WD		(BI2BY*BY2WD)
#define BY2PG		PGSZ
#define PGROUND(s)	ROUNDUP(s, PGSZ)

#define PGSZ		(4*KB)			/* smallest page size */
#define PGSHFT		12			/* log2(PGSZ) */
#define PTSZ		PGSZ			/* page table page size */
/* log(ptes per PTSZ), also virt address bits per PT level */
#define PTSHFT		9
#define PTESHFT		10			/* bits right of PPN in PTE */

#define MACHSZ		PGSZ			/* Mach size, excluding stack */
/*
 * Greatest Mach stack use observed is 1928 bytes (2504 in a rebooted kernel).
 * Note that a single Ureg is 296 bytes.
 * Changing MACHSTKSZ requires recomputing -T for reboottramp; see ktzmkfile.
 */
#define MACHSTKSZ	(3*PGSZ)		/* Mach stack size */

/* Greatest kstack (Proc stack) use observed is 4320 bytes. */
#define KSTACK		(3*PGSZ)		/* Proc kernel stack size */
#define STACKALIGN(sp)	((sp) & ~((uintptr)BY2SE-1)) /* bug: assure with alloc */
/*
 * a system call pushes a Ureg at top of kstack, then a PC below it.
 * this skips past the PC to produce the address of the Ureg.
 */
#define SKIPSTKPC(sp)	((uintptr)(sp) + sizeof(uintptr))

/*
 * Time
 */
#define HZ		(100)			/* clock frequency */
#define MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 *  Address spaces
 *
 *  RV64 Icicle, Unmatched and beagle v implement 39 bits of virtual addresses.
 *  Tinyemu implements both 39 and 48 bits.
 *
 *  Kernel gets loaded at (or just above) 2GB (PHYSMEM, start of ram);
 *  3GB-1MB is used to hold the Sys and Syscomm data structures.
 *
 *  User is at low addresses; kernel vm (high addresses) starts at KZERO.
 */

#define VZERO		0ull
/* VMAPSZ must be multiple of top-level super-page; using PGLSZ ensures that */
#ifdef SV48
/* kseg0 is 0xffff800000000000 */
#define ADDRSPCSZ	(1ull<<(48-1))	/* -1 for high bit, is user vs kernel */
#define VMAPSZ		PGLSZ(3)
#else
/* kseg0 is 0xffffffc000000000 */
#define ADDRSPCSZ	(1ull<<(39-1))	/* -1 for high bit, is user vs kernel */
#define VMAPSZ		PGLSZ(2)
#endif					/* SV48 */

#define	UZERO		0			/* user segment */
#define	UTZERO		(UZERO+PGSZ)		/* first address in user text */
#define UTROUND(t)	ROUNDUP((t), PGSZ)
#define USTKTOP		(ADDRSPCSZ - PGSZ)
#define USTKSIZE	(16*MB)			/* max. size of user stack */
#define TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */

/*
 * PHYSMEM is physical memory base; membanks[0].size is the usable memory
 * directly above that.  this base address is almost guaranteed when a risc-v
 * machine has M & S modes (due to Unix conventions).
 */
#define PHYSMEM		0x80000000u		/* cached */

/*
 *  kernel vm (high addresses) starts at KZERO;
 *  KSEG0 maps the first kernmem bytes, one to one, high to low
 *  (i.e., KSEG0 -> 0), for the kernel's use, rounded up to the nearest GB;
 *  KSEG2 maps all remaining physical memory, and may contain unmapped gaps,
 *  for device registers (or magic ACPI memory, in theory).
 *
 *  Separating KSEG0 and KSEG2 (rather than just having KSEG0 cover everything)
 *  seems to have been intended to simplify start-up on k10 and cover VMAP.
 *
 *  See mmu.c and memory.c.
 */

/*
 * width of virtual addresses determines KSEG0, to allow simple mappings.
 * thus size of virtual addresses is chosen at compile time.
 */
#define KSEG0		(VZERO - ADDRSPCSZ)

/*
 * KZERO is the lowest kernel-space virtual address, and maps to physical 0.
 * KZERO must be a contiguous mask in the highest-order address bits.
 */
#define KZERO		KSEG0

/*
 * kernel usually sits at start of ram (KZERO+PHYSMEM) but some systems are odd
 * and the kernel has to start a little higher.  KTZERO==_main.
 */
#ifndef MAINOFF
#define MAINOFF 0
#endif
#define KTZERO (KZERO + PHYSMEM + MAINOFF) /* kernel text base; see mkfile jl -T */

/* needs to be as much room between KSEG2 and VMAP as membanks[0].size */
#define KSEG2		(KZERO + PHYSMEM + membanks[0].size)

#define isuseraddr(ptr)	((uintptr)(ptr) < KSEG0)

/*
 *  virtual MMU
 */
#define	PTEMAPMEM	(4*MB)	/* memory mapped by a Pte map (arbitrary) */
#define	PTEPERTAB	(PTEMAPMEM/PGSZ) /* pages per Pte map */
#define SSEGMAPSIZE	16		/* initial Ptes per Segment */
/* maximum Segment map size in Ptes. */
#define SEGMAPSIZE (ROUNDUP(USTKTOP, PTEMAPMEM) / PTEMAPMEM) /* <= 262,144 (Sv39) */

/*
 * This is the interface between fixfault and mmuput.
 * Should be in port.  But see riscv.h Pte*.
 */
#define PTEVALID	(1<<0)
#define PTERONLY	(1<<1)
#define PTEWRITE	(1<<2)
#define PTEUSER		(1<<4)		/* unused */
#define PTEUNCACHED	0		/* no such bit on riscv */

#define getpgcolor(a)	0

/*
 * Hierarchical Page Tables.
 * For example, traditional Intel IA-32 paging structures have 2 levels,
 * level 1 is the PD, and level 0 the PT pages; with IA-32e paging,
 * level 3 is the PTROOT, level 2 the PDP, level 1 the PD,
 * and level 0 the PT pages. The PTLX macro gives an index into the
 * page-table page at level 'l' for the virtual address 'v'.
 */
/* 4 for <= 48 bits; 3 for 39 bits */
#define NPGSZ		4	/* max. # of page sizes & page table levels */
#define Npglvls		m->npgsz
#define PGLSHFT(l)	((l)*PTSHFT + PGSHFT)
#define PGLSZ(l)	(1ull << PGLSHFT(l))
#define PTLX(v, l)	(uintptr)(((uvlong)(v) >> PGLSHFT(l)) & VMASK(PTSHFT))

/* this can go when the arguments to mmuput change */
#define PPN(x)		((x) & ~((uintptr)PGSZ-1))
#define PTEPPN(x)	(((uintptr)(x) / PGSZ) << PTESHFT)

#define SYSEXTEND	(64*KB)	/* size of Sys->sysextend */
#define KMESGSIZE	(16*KB)

/* pages allocated by vmap, before malloc is ready */
#define EARLYPAGES	128	/* used: 0 temu, 33 icicle, 32 beaglev */

/* offsets in Sys struct, from dat.h.  originally at start of ram, thus LOW*. */
#define LOW0SYS		(MACHSTKSZ + PTSZ + MACHSZ)
#define LOW0SYSPAGE	(LOW0SYS + PGSZ)

#define HTIFREAD	0x01000000	/* dev 1 cmd 0 */
#define HTIFPRINT	0x01010000	/* dev 1 cmd 1 */

/* verify assertion (cond) at compile time */
#define CTASSERT(cond, name) struct { char name[(cond)? 1: -1]; }
