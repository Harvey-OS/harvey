/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
#define KiB	1024u			/* 0x00000400 */
#define MiB	1048576u		/* 0x00100000 */
#define GiB	1073741824u		/* 0x40000000 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		4096			/* bytes per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 */
#define	HZ		(100)				/* clock frequency */
#define	MS2HZ		(1000/HZ)			/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)			/* ticks to seconds */
#define	TK2MS(x)	((x)*(1000/HZ))
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

/*
 * Fundamental addresses
 * To run in only protected mode (paging off),
 * #define KZERO	0
 * else
 * #define KZERO	0x80000000
 */
#define KZERO		0			/* base of kernel address space */
#define	KTZERO		KZERO			/* first address in kernel text */
#define ROMBIOS		(KZERO|0xF0000)
#define CONFADDR	(KZERO+0x1200)		/* info passed from boot loader */

#define MACHSIZE	(BY2PG*8)			/* stack size */

/*
 *  known 80386 segments (in GDT) and their selectors
 */
#define	NULLSEG		0	/* null segment */
#define	KDSEG		1	/* kernel data/stack */
#define	KESEG		2	/* kernel executable */
#define	UDSEG		3	/* user data/stack */
#define	UESEG		4	/* user executable */
#define	SYSGATE		5	/* system call gate */
#define TSSSEG		6	/* task segment */
#define	APMCSEG		6	/* APM code segment */
#define	APMCSEG16	7	/* APM 16-bit code segment */
#define	APMDSEG		8	/* APM data segment */
#define	NGDT		10	/* number of GDT entries required */

#define SELGDT	(0<<3)	/* selector is in gdt */
#define	SELLDT	(1<<3)	/* selector is in ldt */

#define SELECTOR(i, t, p)	(((i)<<3) | (t) | (p))

#define NULLSEL	SELECTOR(NULLSEG, SELGDT, 0)
#define KESEL	SELECTOR(KESEG, SELGDT, 0)
#define KDSEL	SELECTOR(KDSEG, SELGDT, 0)
#define UESEL	SELECTOR(UESEG, SELGDT, 3)
#define UDSEL	SELECTOR(UDSEG, SELGDT, 3)
#define TSSSEL	SELECTOR(TSSSEG, SELGDT, 0)

/*
 *  fields in segment descriptors
 */
#define SEGDATA	(0x10<<8)	/* data/stack segment */
#define SEGEXEC	(0x18<<8)	/* executable segment */
#define	SEGTSS	(0x9<<8)	/* TSS segment */
#define SEGCG	(0x0C<<8)	/* call gate */
#define	SEGIG	(0x0E<<8)	/* interrupt gate */
#define SEGTG	(0x0F<<8)	/* task gate */
#define SEGTYPE	(0x1F<<8)

#define SEGP	(1<<15)		/* segment present */
#define SEGPL(x) ((x)<<13)	/* priority level */
#define SEGB	(1<<22)		/* granularity 1==4k (for expand-down) */
#define SEGG	(1<<23)		/* granularity 1==4k (for other) */
#define SEGE	(1<<10)		/* expand down */
#define SEGW	(1<<9)		/* writable (for data/stack) */
#define	SEGR	(1<<9)		/* readable (for code) */
#define SEGD	(1<<22)		/* default 1==32bit (for code) */

/*
 *  virtual MMU
 */
#define PTEMAPMEM	(1024*1024)	/* ??? */
#define SEGMAPSIZE	16		/* ??? */
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)	/* ??? */
#define PPN(x)		((x)&~(BY2PG-1))

/*
 *  physical MMU
 */
#define	PTEVALID	(1<<0)
#define	PTEUNCACHED	0		/* everything is uncached */
#define PTEWRITE	(1<<1)
#define	PTERONLY	(0<<1)
#define	PTEKERNEL	(0<<2)
#define	PTEUSER		(1<<2)
#define PTESIZE		(1<<7)

/*
 *  flag register bits that we care about
 */
#define IFLAG	0x200
