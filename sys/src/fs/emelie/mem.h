#define	BY2WD		4			/* bytes per word */
#define BY2V		8			/* bytes per vlong */
#define	BY2PG		4096			/* bytes per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))
#define MB		(1024*1024)

#define	HZ		(82)			/* clock frequency */
#define	TK2MS(t)	(((ulong)(t)*1000)/HZ)	/* ticks to milliseconds - beware rounding */
#define	MS2TK(t)	(((ulong)(t)*HZ)/1000)	/* milliseconds to ticks - beware rounding */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 * Fundamental addresses
 */
#define IDTADDR		0x80000800		/* idt */
#define APBOOTSTRAP	0x80001000		/* AP bootstrap code */
#define CONFADDR	0x80001200		/* info passed from boot loader */
#define CPU0PDB		0x80002000		/* bootstrap processor PDB */
#define CPU0PTE		0x80003000		/* bootstrap processor PTE's for 0-2MB */
#define CPU0MACHPTE	0x80004000		/* bootstrap processor PTE for MACHADDR */
#define CPU0MACH	0x80005000		/* Mach for bootstrap processor */

#define	KZERO		0x80000000		/* base of kernel address space */
#define	KTZERO		0x80100000		/* first address in kernel text */

#define	MACHSIZE	4096

/*
 *  known 80386 segments (in GDT) and their selectors
 */
#define	NULLSEG	0	/* null segment */
#define	KDSEG	1	/* kernel data/stack */
#define	KESEG	2	/* kernel executable */	
#define	UDSEG	3	/* user data/stack */
#define	UESEG	4	/* user executable */
#define TSSSEG	5	/* task segment */
#define N386SEG	6	/* number of segments */

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
 *  physical MMU
 */
#define	PTEVALID	(1<<0)
#define	PTEUNCACHED	(1<<4)	
#define PTEWRITE	(1<<1)
#define	PTERONLY	(0<<1)
#define	PTEKERNEL	(0<<2)
#define	PTEUSER		(1<<2)
#define PTESIZE		(1<<7)

#define MACHADDR	((ulong)&mach0)		/* hack number 1 */

#define IFLAG		0x200
