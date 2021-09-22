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
#ifndef MB
#define KB		1024
#define MB		(1024*KB)
#define GB		(1024*MB)
#endif
#define	BY2XPG		(4096*KB)		/* bytes per big page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define	XPGSHIFT	22			/* log(BY2XPG) */

#define	BLOCKALIGN	8
#define FPalign		16			/* required for FXSAVE */

#ifndef MAXMACH
#define	MAXMACH		64			/* max # cpus system can run */
#endif
/* max. seen used is 2788 bytes */
#ifndef KSTACK
#define	KSTACK		BY2PG			/* Size of kernel stack */
#endif
#define MACHSIZE	BY2PG			/* mode32bit knows this size */

/*
 * Time
 */
#define	HZ		(100)			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 *  Address spaces
 */
/* KZERO must match def'n in mkfile */
#ifndef KZERO
#define	KZERO		0xE0000000		/* base of kernel address space */
#define KSEGM		0xE0000000		/* kernel/user space bits mask */
#define	KTZERO		(KZERO+0x100000)	/* first address in kernel text */
#endif
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
#define	USTKSIZE	(16*MB)			/* size of user stack */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */
#define	TSTKSIZ 	256	/* pages in new stack; limits exec args */

/* verify assertion (cond) at compile time */
#define CTASSERT(cond, name) struct { char name[(cond)? 1: -1]; }

/*
 * Fundamental addresses - bottom 64kB saved for return to real mode
 *
 * first 1K is real-mode IVT (vectors), 0—0x3ff.
 */
#define VectorSYSCALL	64
/* these are not used by assembler */
#define BIOSDATA	(KZERO+0x400)	/* through 0x4ff */
#define EBDAADDR	(KZERO+0x40e)	/* must not step on EBDA */
#define FBMADDR		(KZERO+0x413) /* free base mem (KB); misaligned short */
#define WARMRESET	(KZERO+0x467)	/* warm-reset vector (misaligned) */
#define WARMBOOT	(KZERO+0x472)	/* short flag; 0x1234 asks warm boot */
#define NBIOSDRIVES	(KZERO+0x475)	/* uchar: # of drives known to bios */

/* CONFADDR, MBOOTTAB & BIOSTABLES are used by ../pcboot to pass config data */
#define MBOOTTAB	(KZERO+0x500)	/* multiboot tables, if present */
#define CONFADDR	(KZERO+0x1200)	/* info passed from boot loader */
#define TMPADDR		(KZERO+0x2000)	/* used for temporary mappings */
/* APBOOTSTRAP must match def'n in mkfile */
#define APBOOTSTRAP	(KZERO+0x3000)	/* AP bootstrap code [178; within 1st 64K] */
			/* pages 4-6 unused */
/*
 * these apm & e820 tables are generated early, in real mode (e.g., by expand),
 * then chewed up by readlsconf() in conf.c to produce other data structures.
 * only of use if we are booted directly by pxe (thus expand), not via 9boot;
 * 9boot will have placed equivalent multiboot tables at MBOOTTAB.
 */
#define BIOSTABLES	(KZERO+0x7000)	/* apm & e820 from real mode [1K] */
#define PXEBASE		0x7c00		/* phys: pxe loads us here */
#define RMSTACK		PXEBASE		/* phys: real stack base [2K below] */
#define RMUADDR		(KZERO+0x7C00)	/* real mode Ureg */
#define RMCODE		(KZERO+0x8000)	/* copy of 1st page of kernel text */
/*
 * buffer for user space - known to aux/vga/vesa.c, thus not trivially changed.
 * at most 1 page for /dev/realmode.  holds a single e820 map entry.
 */
#define RMBUF		(KZERO+0x9000)	/* buffer for user space */
			/* 26K gap here, pages 0xA-0xF unused */

/* 0x10000 is 64K */
#define IDTADDR		(KZERO+0x10800)	/* idt */
/* REBOOTADDR must match def'n in mkfile */
#define REBOOTADDR	0x11000		/* reboot code - physical address; 65 */
/*
 * N.B.  lowraminit & map know that CPU0END is the end of reserved data
 */
#define CPU0START	CPU0GDT
#define CPU0GDT		(KZERO+0x12000)	/* bootstrap processor GDT */
#define MACHADDR	(KZERO+0x13000)	/* as seen by current processor */
#define CPU0MACH	(KZERO+0x14000)	/* Mach for bootstrap processor */
#define CPU0PDB		(KZERO+0x15000)	/* bootstrap processor PDB */
#define CPU0PTE		(KZERO+0x16000)	/* bootstrap processor MemMin PTEs */
#define CPU0PTEEND	(CPU0PTE + LOWPTEPAGES*BY2PG)
#define CPU0END		CPU0PTEEND

#define LOWMEMEND	(KZERO+0xa0000)
#define CGAMEM		(KZERO+0xB8000)	/* cga display memory */
#define BIOSROMS	(KZERO+0xe0000)

#define Maxmmap 32			/* nelem(mmap) - 1 */

/*
 * Where configuration info is left for the loaded programme.
 * There are 3,584 bytes available at CONFADDR, which is ample.
 */
#define BOOTLINE	((char*)CONFADDR)
#define BOOTLINELEN	64
#define BOOTARGS	((char*)(CONFADDR+BOOTLINELEN))
#define	BOOTARGSLEN	(BY2PG - (uintptr)BOOTARGS % BY2PG)
#define	MAXCONF		100

/* multiboot magic */
#define	MBOOTHDRMAG	0x1badb002
#define MBOOTREGMAG	0x2badb002

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
/* #define APM40SEG	8	/* APM segment 0x40 */

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
/* #define APM40SEL	SELECTOR(APM40SEG, SELGDT, 0) */

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

#define	SEGG	(1<<23)		/* granularity 1==4k (for other) */
#define	SEGB	(1<<22)		/* granularity 1==4k (for expand-down) */
#define	SEGD	(1<<22)		/* default 1==32bit (for code) */
#define	SEGP	(1<<15)		/* segment present */
#define	SEGPL(x) ((x)<<13)	/* priority level */
#define	SEGE	(1<<10)		/* expand down */
#define	SEGR	(1<<9)		/* readable (for code) */
#define	SEGW	(1<<9)		/* writable (for data/stack) */

/*
 *  virtual MMU
 */
#define	PTEMAPMEM	MB		/* memory mapped by a Pte map */
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)	/* pages per Pte map */
/* maximum segment map size in Ptes. */
#define	SEGMAPSIZE	(ROUND(USTKTOP, PTEMAPMEM) / PTEMAPMEM)
#define	SSEGMAPSIZE	16	/* starting (default) segment map size */
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

/* CR0: processor control */
#define PE	(1<<0)		/* protected mode enable */
#define MP	(1<<1)		/* monitor coprocessor */
#define EM	(1<<2)		/* fp emulation */
#define TS	(1<<3)		/* task switched */
#define NE	(1<<5)		/* numeric error */
#define WP	(1<<16)		/* write protect */
#define NW	(1<<29)		/* not write-through caching */
#define CD	(1<<30)		/* cache disable */
#define PG	(1<<31)		/* paging enable */

/* eflags */
#define IF	0x200		/* interrupt enable flag (see STI, CLI) */
#define Ac	0x40000		/* alignment check */
#define Id	0x200000	/* have cpuid instr. */

/*
 * Macros for calculating offsets within the page directory base
 * and page tables.
 */
#define	PDX(va)		((((ulong)(va))>>XPGSHIFT) & 0x03FF)
#define	PTX(va)		((((ulong)(va))>>PGSHIFT) & 0x03FF)

#define	getpgcolor(a)	0

/* prominent i/o ports */
#define KBDATA		0x60	/* data port */
#define KBDCTLB		0x61	/* keyboard control B i/o port; timer 2 ctl */
#define KBDCMD		0x64	/* command/status port (write only) */

#define NVRADDR		0x70	/* nvram/rtc address port */
#define NVRDATA		0x71	/* nvram/rtc data port */
#define FPCLRLATCH	0xf0	/* a write clears intr latch from the 387 */

#define Sysctla		0x92	/* system control port a */
#define Sysctlreset	(1<<0)	/* " bits */
#define Sysctla20ena	(1<<1)

/*
 * MemMin is what the bootstrap code in l*.s has already mapped.
 */
#ifndef MemMin
#define MemMin	(8*MB)		/* don't have PTEs for more allocated yet */
#endif

/* for boot-loading kernels and expand only */
/* normal kernel decompresses to MB; boot kernel decompressed to 9*MB */
#define Kernelmax  (8*MB)	/* max size of normal kernel, not an address */
#define Mallocbase (16*MB)

#define LOWPTEPAGES (MemMin / (4*MB))
