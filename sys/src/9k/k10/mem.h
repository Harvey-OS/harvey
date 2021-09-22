/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
#define TOS(stktop)	((stktop)-sizeof(Tos))

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
#define BI2BY		8			/* bits per byte */
#define BY2V		8			/* bytes per double word */
#define BY2SE		8			/* bytes per stack element */
#define BLOCKALIGN	8
#define FPalign		16			/* required for FXSAVE */
/* old names */
#define BY2WD		4
#define BI2WD		(BI2BY*BY2WD)
#define BY2PG		PGSZ
#define PGROUND(s)	ROUNDUP(s, PGSZ)

#define PGSZ		(4*KB)			/* smallest page size */
#define PGSHFT		12			/* log(PGSZ) */
#define PTSZ		PGSZ			/* page table page size */
/* log(ptes per PTSZ), also address bits per PT level */
#define PTSHFT		9

#define MACHSZ		PGSZ			/* Mach size, excluding stack */
#define MACHSTKSZ	(6*PGSZ)		/* Mach stack size */
/*
 * max. # of cpus (less than 256 [apics], was 32); costs 1 byte/cpu in
 * sizeof(Page)
 */
#define MACHMAX		16

#define KSTACK		(16*1024)		/* Size of Proc kernel stack */
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
 *  AMD64 implementations currently implement 48 bits of addresses,
 *  with 128TB at the top of 64-bit space and 128TB at 0.
 *
 *  Kernel gets loaded at 1MB+128K;
 *  Memory from 0 to 1MB is not used for other things;
 *  1MB to 1MB+128K is used to hold the Sys and
 *  Syscomm datastructures.
 *
 *  User is at low addresses; kernel vm starts at KZERO;
 *  KSEG0 maps the first kernmem bytes, one to one (i.e., KZERO -> 0),
 *  for the kernel's use, rounded up to the nearest GB;
 *  KSEG1 maps the PML4 into itself;
 *  KSEG2 maps all remaining physical memory, and may contain unmapped gaps,
 *  notably below 4GB, for device registers or magic ACPI memory.  For example,
 *  given that below 0x60000000 (1.5GB) is reserved for the kernel,
 *
 *   0x060000000 0x081b68000 mem   (565608448) va 0xfffffe0060000000
 *   0x081b6a000 0x08a82c000 mem   (147595264) va 0xfffffe0081b6a000
 *   0x08d2a2000 0x08d300000 mem      (385024) va 0xfffffe008d2a2000
 *   0x100000000 0x46f000000 mem (14747172864) va 0xfffffe0100000000
 *
 *  Separating KSEG0 and KSEG2 (rather than just having KSEG0 cover everything)
 *  seems to be intended to simplify start-up (l32p.s and l16sipi.s) and cover
 *  VMAP.
 *
 *  See mmu.c and asm.c.
 */
#define UTZERO		(0+2*MB)		/* first address in user text */
#define UTROUND(t)	ROUNDUP((t), 2*MB)
#define USTKTOP		(128*TB - PGSZ)
#define USTKSIZE	(16*MB)			/* max. size of user stack */
#define TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */

/* verify assertion (cond) at compile time */
#define CTASSERT(cond, name) struct { char name[(cond)? 1: -1]; }

/*
 * 2GB is about all we can be sure of having below 4GB these days, though we
 * could consult the e820 map.  Above that we might run into memory-mapped
 * registers or magic untouchable ACPI memory.
 */
#define KSEG0SIZE	(2ull*GB)	/* update mkfile if this changes */

#define VZERO		0ull
#define PMAPADDR	(VZERO-(2*MB))		/* unused as of yet (KMAP?) */

#define KSEG0		(VZERO-KSEG0SIZE)	/* 2GB */
#define KSEG1		(VZERO-TB)		/* 512GB - embedded PML4 only */
#define KSEG2		(VZERO-128*TB)		/* up to KMAP (PMAPADDR) */

#define KZERO		KSEG0
#define KTZERO		(KZERO+1*MB+128*KB)	/* see mkfile 6l -T */

#define isuseraddr(ptr)	((uintptr)(ptr) < KSEG2)

/*
 *  virtual MMU
 */
#define	PTEMAPMEM	MB	/* memory mapped by a Pte map (arbitrary) */
#define	PTEPERTAB	(PTEMAPMEM/PGSZ) /* pages per Pte map */
#define SSEGMAPSIZE	16		/* initial Ptes per Segment */
/* maximum Segment map size in Ptes. */
#define SEGMAPSIZE (ROUNDUP(USTKTOP, PTEMAPMEM) / PTEMAPMEM) /* 134,217,728 */

/*
 * This is the interface between fixfault and mmuput.
 * Should be in port.
 */
#define PTEVALID	(1<<0)
#define PTEWRITE	(1<<1)
#define PTERONLY	(0<<1)
#define PTEUSER		(1<<2)
#define PTEUNCACHED	(1<<4)

#define getpgcolor(a)	0

/*
 * Hierarchical Page Tables.
 * For example, traditional IA-32 paging structures have 2 levels,
 * level 1 is the PD, and level 0 the PT pages; with IA-32e paging,
 * level 3 is the PML4(!), level 2 the PDP, level 1 the PD,
 * and level 0 the PT pages. The PTLX macro gives an index into the
 * page-table page at level 'l' for the virtual address 'v'.
 */
#define Npglvls		4		/* for 48 bits; could be 5 some day */
#define PTLX(v, l)	(((v) >> (((l)*PTSHFT)+PGSHFT)) & ((1<<PTSHFT)-1))
#define PGLSZ(l)	(1 << (((l)*PTSHFT)+PGSHFT))

/* this can go when the arguments to mmuput change */
#define PPN(x)		((x) & ~((uintptr)PGSZ-1))

/* prominent i/o ports */
#define KBDATA		0x60	/* data port */
#define KBDCTLB		0x61	/* keyboard control B i/o port; timer 2 ctl */
#define KBDCMD		0x64	/* command/status port (write only) */

#define NVRADDR		0x70	/* nvram/rtc address port */
#define NVRDATA		0x71	/* nvram/rtc data port */
#define FPCLRLATCH	0xf0	/* a write clears intr latch from the 387 */

#define SYSEXTEND	(64*KB)	/* size of Sys->sysextend */
#define KMESGSIZE	(16*KB)

/*
 * buffer for user space - known to aux/vga/vesa.c, thus not trivially changed.
 * at most 1 page for /dev/realmode.  holds a single e820 map entry.
 * TODO: this is from 9/pc and may be wrong for 9k/k10.
 */
#define RMBUF		(KZERO+0x9000)	/* buffer for user space */

/* offsets in Sys struct at 1MB (before kernel), from dat.h */
#define LOWMACHSTK	0
#define LOWPML4		(LOWMACHSTK + MACHSTKSZ)	/* page tables */
#define LOWPDP		(LOWPML4 + PTSZ)
#define LOWPD		(LOWPDP + PTSZ)
#define LOWPT		(LOWPD + PTSZ)
#define LOWVSM		(LOWPT + PTSZ)
#define LOWMACH		(LOWVSM + PGSZ)
/* rest are on cpu0 only */
#define LOW0SYS		(LOWMACH + MACHSZ)
#define LOW0MACHPTR	(LOW0SYS + PGSZ)
#define LOW0PD2G	(LOW0MACHPTR + PGSZ)
#define LOW0PT2G	(LOW0PD2G + PTSZ)
#define LOW0END		(LOW0PT2G + PTSZ + SYSEXTEND)
#define LOW0ZEROEND	(LOW0END - KMESGSIZE)	/* stop zeroing before Kmesg */

/* sipihandler code addr, with Sipi array in next pages; extends up to 0x6000 */
#define SIPIHANDLER	(KZERO+0x3000)		/* see mkfile 6l -T */

/* offsets into struct Sipi from dat.h */
#define SIPIPML4 0
#define SIPIARGS BY2WD
#define SIPISTK  BY2V
#define SIPIMACH (2*BY2V)
#define SIPIPC	 (3*BY2V)
#define SIPISIZE (4*BY2V)

#define RBTRAMP 0
#define RBPHYSENTRY BY2V
#define RBSRC	(2*BY2V)
#define RBLEN	(3*BY2V)

#define L16GET(p)	(((p)[1]<<8)|(p)[0])
#define EBDAADDR	(KZERO+0x40e)	/* must not step on EBDA */
#define LOWMEMEND	(KZERO+0xa0000)	/* end usable RAM below 1MB */

/* multiboot magic */
#define	MBOOTHDRMAG	0x1badb002
#define MBOOTREGMAG	0x2badb002

#define MBOOTTAB	(KZERO+0x500)	/* multiboot tables, if present */

/*
 * global lapicbase is a kernel virtual addr.;
 * we need the physical addr. in the msi addr. reg.
 */
#define Lapicphys 0xfee00000ul

#undef SDATA
