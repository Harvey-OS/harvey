/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2V		8			/* bytes per double word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))
#define PGROUND(s)	ROUND(s, BY2PG)
#define	BLOCKALIGN	8

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 */
#define	HZ		(20)				/* clock frequency */
#define	MS2HZ		(1000/HZ)			/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)			/* ticks to seconds */
#define	TK2MS(t)	((((ulong)(t))*1000)/HZ)	/* ticks to milliseconds */
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

/*
 *  Virtual addresses:
 *
 *  We direct map all discovered DRAM and the area twixt 0xe0000000 and
 *  0xe8000000 used to provide zeros for cache flushing.
 *
 *  Flash is mapped to 0xb0000000 and special registers are mapped
 *  on demand to areas starting at 0xa0000000.
 *
 *  The direct mapping is convenient but not necessary.  It means
 *  that we don't have to turn on the MMU till well into the
 *  kernel.  This can be changed by providing a mapping in l.s
 *  before calling main.
 */
#define	UZERO		0			/* base of user address space */
#define	UTZERO		(UZERO+BY2PG)		/* first address in user text */
#define	KZERO		0xC0000000		/* base of kernel address space */
#define	KTZERO		0xC0008000		/* first address in kernel text */
#define	EMEMZERO	0x90000000		/* 256 meg for add on memory */
#define	EMEMTOP		0xA0000000		/* ... */
#define	REGZERO		0xA0000000		/* 128 meg for mapspecial regs */
#define	REGTOP		0xA8000000		/* ... */
#define	FLASHZERO	0xB0000000		/* 128 meg for flash */
#define	FLASHTOP	0xB8000000		/* ... */
#define	DRAMZERO	0xC0000000		/* 128 meg for dram */
#define DRAMTOP		0xC8000000		/* ... */
#define	UCDRAMZERO	0xC8000000		/* 128 meg for dram (uncached/unbuffered) */
#define UCDRAMTOP	0xD0000000		/* ... */
#define	NULLZERO	0xE0000000		/* 128 meg for cache flush zeroes */
#define NULLTOP		0xE8000000		/* ... */
#define	USTKTOP		0x2000000		/* byte just beyond user stack */
#define	USTKSIZE	(8*1024*1024)		/* size of user stack */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* end of new stack in sysexec */
#define TSTKSIZ 	100
#define MACHADDR	(KZERO+0x00001000)
#define	EVECTORS	0xFFFF0000		/* virt base of exception vectors */

#define KSTACK		(16*1024)		/* Size of kernel stack */

/*
 *  Offsets into flash
 */
#define Flash_bootldr	(FLASHZERO+0x0)		/* boot loader */
#define Flash_kernel	(FLASHZERO+0x10000)	/* boot kernel */
#define	Flash_tar	(FLASHZERO+0x100000)	/* tar file containing fs.sac */

/*
 *  virtual MMU
 */
#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	1984
#define SSEGMAPSIZE	16
#define PPN(x)		((x)&~(BY2PG-1))

/*
 *  SA1110 definitions
 */

/*
 *  memory physical addresses
 */
#define PHYSFLASH0	0x00000000
#define PHYSDRAM0	0xC0000000
#define	PHYSNULL0	0xE0000000

/*
 *  peripheral control module physical addresses
 */
#define USBREGS		0x80000000	/* serial port 0 - USB */
#define UART1REGS	0x80010000	/* serial port 1 - UART */
#define GPCLKREGS	0x80020060	/* serial port 1 - general purpose clock */
#define UART2REGS	0x80030000	/* serial port 2 - low speed IR */
#define HSSPREGS	0x80040060	/* serial port 2 - high speed IR */
#define UART3REGS	0x80050000	/* serial port 3 - RS232 UART */
#define MCPREGS		0x80060000	/* serial port 4 - multimedia comm port */
#define SSPREGS		0x80070060	/* serial port 4 - synchronous serial port */
#define OSTIMERREGS	0x90000000	/* operating system timer registers */
#define POWERREGS	0x90020000	/* power management */
#define GPIOREGS	0x90040000	/* 28 general purpose IO pins */
#define INTRREGS	0x90050000	/* interrupt registers */
#define PPCREGS		0x90060000	/* peripheral pin controller */
#define MEMCONFREGS	0xA0000000	/* memory configuration */
#define LCDREGS		0xB0100000	/* display */

/*
 *  PCMCIA addresses
 */
#define PHYSPCM0REGS	0x20000000
#define PYHSPCM0ATTR	0x28000000
#define PYHSPCM0MEM	0x2C000000
#define PHYSPCM1REGS	0x30000000
#define PYHSPCM1ATTR	0x38000000
#define PYHSPCM1MEM	0x3C000000

/*
 *  Program Status Registers
 */
#define PsrMusr		0x00000010	/* mode */
#define PsrMfiq		0x00000011
#define PsrMirq		0x00000012
#define PsrMsvc		0x00000013
#define PsrMabt		0x00000017
#define PsrMund		0x0000001B
#define PsrMask		0x0000001F

#define PsrDfiq		0x00000040	/* disable FIQ interrupts */
#define PsrDirq		0x00000080	/* disable IRQ interrupts */

#define PsrV		0x10000000	/* overflow */
#define PsrC		0x20000000	/* carry/borrow/extend */
#define PsrZ		0x40000000	/* zero */
#define PsrN		0x80000000	/* negative/less than */

/*
 *  Coprocessors
 */
#define CpMMU		15
#define CpPWR		15

/*
 *  Internal MMU coprocessor registers
 */
#define CpCPUID		0		/* R: */
#define CpControl	1		/* R: */
#define CpTTB		2		/* RW: translation table base */
#define CpDAC		3		/* RW: domain access control */
#define CpFSR		5		/* RW: fault status */
#define CpFAR		6		/* RW: fault address */
#define CpCacheFlush	7		/* W: cache flushing, wb draining*/
#define CpTLBFlush	8		/* W: TLB flushing */
#define CpRBFlush	9		/* W: Read Buffer ops */
#define CpPID		13		/* RW: PID for virtual mapping */
#define	CpBpt		14		/* W: Breakpoint register */
#define CpTest		15		/* W: Test, Clock and Idle Control */

/*
 *  CpControl
 */
#define CpCmmuena	0x00000001	/* M: MMU enable */
#define CpCalign	0x00000002	/* A: alignment fault enable */
#define CpCdcache	0x00000004	/* C: data cache on */
#define CpCwb		0x00000008	/* W: write buffer turned on */
#define CpCi32		0x00000010	/* P: 32-bit program space */
#define CpCd32		0x00000020	/* D: 32-bit data space */
#define CpCbe		0x00000080	/* B: big-endian operation */
#define CpCsystem	0x00000100	/* S: system permission */
#define CpCrom		0x00000200	/* R: ROM permission */
#define CpCicache	0x00001000	/* I: instruction cache on */
#define CpCvivec	0x00002000	/* X: virtual interrupt vector adjust */

/*
 *  fault codes
 */
#define	FCterm		0x2	/* terminal */
#define	FCvec		0x0	/* vector */
#define	FCalignf	0x1	/* unaligned full word data access */
#define	FCalignh	0x3	/* unaligned half word data access */
#define	FCl1abort	0xc	/* level 1 external abort on translation */
#define	FCl2abort	0xe	/* level 2 external abort on translation */
#define	FCtransSec	0x5	/* section translation */
#define	FCtransPage	0x7	/* page translation */
#define	FCdomainSec	0x9	/* section domain  */
#define	FCdomainPage	0x11	/* page domain */
#define	FCpermSec	0x9	/* section permissions  */
#define	FCpermPage	0x11	/* page permissions */
#define	FCabortLFSec	0x4	/* external abort on linefetch for section */
#define	FCabortLFPage	0x6	/* external abort on linefetch for page */
#define	FCabortNLFSec	0x8	/* external abort on non-linefetch for section */
#define	FCabortNLFPage	0xa	/* external abort on non-linefetch for page */

/*
 *  PTE bits used by fault.h.  mmu.c translates them to real values.
 */
#define	PTEVALID	(1<<0)
#define	PTERONLY	0	/* this is implied by the absence of PTEWRITE */
#define	PTEWRITE	(1<<1)
#define	PTEUNCACHED	(1<<2)
#define PTEKERNEL	(1<<3)	/* no user access */

/*
 *  H3650 specific definitions
 */
#define EGPIOREGS	0x49000000	/* Additional GPIO register */
