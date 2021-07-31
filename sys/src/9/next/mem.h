/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		8192			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		13			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 * Clock frequency is 68.3900 HZ
 */
#define	HZ		(68)			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	muldiv(t,100,6839)	/* ticks to seconds */
#define	TK2MS(t)	muldiv(t,100000,6839)	/* ticks to milliseconds */
#define	MS2TK(t)	muldiv(t,6839,100000)	/* milliseconds to ticks */

/*
 * SR bits
 */
#define SUPER		0x2000
#define SPL(n)		(n<<8)

/*
 * CACR
 */
#define	CDISABLE	0x00000000
#define	CENABLE		0x80008000

/*
 * Magic registers (unused in current system)
 */

#define	MACH		A5		/* A5 is m-> */
#define	USER		A4		/* A4 is u-> */

/*
 * Fundamental addresses
 */

#define	USERADDR	KIOBTOP
/* assuming we're in a syscall, this is the address of the Ureg structure */
#define	UREGVARSZ	(15*BY2WD)	/* size of variable part of Ureg */
#define	UREGADDR	(USERADDR+BY2PG-(UREGVARSZ+2+4+2+(8+8+1+1)*BY2WD))

/*
 * MMU
 */

#define	VAMASK	0xFFFFFFFF
#define	KUSEG	0x00000000
#define KSEG	0x04000000

/*
 * MMU entries
 */
#define	PTEVALID	((1<<5)|(1<<0))		/* copyback, valid */
#define	PTEUNCACHED	((2<<5)|(1<<0))		/* uncached, serial, valid */
#define PTEWRITE	0
#define	PTERONLY	(1<<2)
#define	PTEKERNEL	(1<<7)
#define	INVALIDPTE	0
#define PPN(x)		((x)&~(BY2PG-1))
#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	64
#define PTEAMASK	(~((1<<9)-1))		/* address portion of A level pte */
#define PTEBMASK	(~((1<<7)-1))		/* address portion of B level pte */

/*
 * MMU descriptors
 */
#define	DESCVALID	(2<<0)
#define	INVALIDDESC	0

#define	KMAP	((unsigned long *)0xD0000000)
#define	UMAP	((unsigned long *)0x50000000)


/*
 * Address spaces
 *
 * User is at 0-64MB
 * Kernel is at 64-128MB through ITT0, DTT0
 * Video is mapped through DTT1 in l.s
 * I/O is mapped starting at address 128MB
 */

#define	UZERO	KUSEG			/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	TSTKTOP	KSEG			/* end of new stack in sysexec */
#define TSTKSIZ 10
#define	USTKTOP	(TSTKTOP-TSTKSIZ*BY2PG)	/* byte just beyond user stack */
#define	KZERO	KSEG			/* base of kernel address space */
#define	KTZERO	KZERO			/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */
#define	KIO	0x08000000		/* 128MB virtual is I/O space */
#define	KIOTOP	(KIO+1024*1024)		/* 1MB is enough for I/O */
#define	KIOB	0x0A000000		/* another 128MB for byte space */
#define	KIOBTOP	(KIOB+256*1024)		/* 256K is enough for I/O */

#define	MACHSIZE	4096

#define isphys(x) (((ulong)(x)&0xFE000000) == KZERO)
