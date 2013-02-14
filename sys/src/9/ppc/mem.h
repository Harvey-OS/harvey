/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

#ifdef ucuconf
#include "ucu.h"
#else
#include "blast.h"
#endif

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define	BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define BY2V		8			/* bytes per vlong */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define	CACHELINELOG	5
#define	CACHELINESZ	(1<<CACHELINELOG)
#define	BLOCKALIGN	CACHELINESZ

#define	MHz		1000000

#define	BY2PTE		8			/* bytes per pte entry */
#define	BY2PTEG		64			/* bytes per pte group */

#define	MAXMACH	1				/* max # cpus system can run */
#define	MACHSIZE	BY2PG
#define	KSTACK		4096			/* Size of kernel stack */

/*
 * Time
 */
#define	HZ		1000			/* clock frequency */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */

/*
 * Standard PPC Special Purpose Registers (OEA and VEA)
 */
#define DSISR	18
#define DAR	19		/* Data Address Register */
#define DEC	22		/* Decrementer */
#define SDR1	25
#define SRR0	26		/* Saved Registers (exception) */
#define SRR1	27
#define TBRL	268
#define TBRU	269		/* Time base Upper/Lower (Reading) */
#define SPRG0	272		/* Supervisor Private Registers */
#define SPRG1	273
#define SPRG2	274
#define SPRG3	275
#define SPRG4	276
#define SPRG5	277
#define SPRG6	278
#define SPRG7	279
#define ASR	280		/* Address Space Register */
#define EAR	282		/* External Access Register (optional) */
#define TBWU	284		/* Time base Upper/Lower (Writing) */
#define TBWL	285
#define PVR	287		/* Processor Version */
#define IABR	1010	/* Instruction Address Breakpoint Register (optional) */
#define DABR	1013	/* Data Address Breakpoint Register (optional) */
#define FPECR	1022	/* Floating-Point Exception Cause Register (optional) */
#define PIR	1023	/* Processor Identification Register (optional) */

#define IBATU(i)	(528+2*(i))	/* Instruction BAT register (upper) */
#define IBATL(i)	(529+2*(i))	/* Instruction BAT register (lower) */
#define DBATU(i)	(536+2*(i))	/* Data BAT register (upper) */
#define DBATL(i)	(537+2*(i))	/* Data BAT register (lower) */

/*
 * PPC604e-specific Special Purpose Registers (OEA)
 */
#define MMCR0		952		/* Monitor Control Register 0 */
#define PMC1		953		/* Performance Monitor Counter 1 */
#define PMC2		954		/* Performance Monitor Counter 2 */
#define SIA		955		/* Sampled Instruction Address */
#define MMCR1		956		/* Monitor Control Register 0 */
#define PMC3		957		/* Performance Monitor Counter 3 */
#define PMC4		958		/* Performance Monitor Counter 4 */
#define SDA		959		/* Sampled Data Address */
/*
 * PPC603e-specific Special Purpose Registers
 */
#define DMISS		976		/* Data Miss Address Register */
#define DCMP		977		/* Data Miss Address Register */
#define HASH1		978
#define HASH2		979
#define IMISS		980		/* Instruction Miss Address Register */
#define iCMP		981		/* Instruction Miss Address Register */
#define RPA		982
#define HID0		1008	/* Hardware Implementation Dependent Register 0 */
#define HID1		1009	/* Hardware Implementation Dependent Register 1 */
/*
 * PowerQUICC II (MPC 8260) Special Purpose Registers
 */
#define HID2		1011	/* Hardware Implementation Dependent Register 2 */

#define	BIT(i)		(1<<(31-(i)))	/* Silly backwards register bit numbering scheme */
#define	SBIT(n)		((ushort)1<<(15-(n)))
#define	RBIT(b,n)	(1<<(8*sizeof(n)-1-(b)))

/*
 * Bit encodings for Machine State Register (MSR)
 */
#define MSR_POW		BIT(13)		/* Enable Power Management */
#define MSR_TGPR	BIT(14)		/* Temporary GPR Registers in use (603e) */
#define MSR_ILE		BIT(15)		/* Interrupt Little-Endian enable */
#define MSR_EE		BIT(16)		/* External Interrupt enable */
#define MSR_PR		BIT(17)		/* Supervisor/User privilege */
#define MSR_FP		BIT(18)		/* Floating Point enable */
#define MSR_ME		BIT(19)		/* Machine Check enable */
#define MSR_FE0		BIT(20)		/* Floating Exception mode 0 */
#define MSR_SE		BIT(21)		/* Single Step (optional) */
#define MSR_BE		BIT(22)		/* Branch Trace (optional) */
#define MSR_FE1		BIT(23)		/* Floating Exception mode 1 */
#define MSR_IP		BIT(25)		/* Exception prefix 0x000/0xFFF */
#define MSR_IR		BIT(26)		/* Instruction MMU enable */
#define MSR_DR		BIT(27)		/* Data MMU enable */
#define MSR_PM		BIT(29)		/* Performance Monitor marked mode (604e specific) */
#define MSR_RI		BIT(30)		/* Recoverable Exception */
#define MSR_LE		BIT(31)		/* Little-Endian enable */
/* SRR1 bits for TLB operations */
#define MSR_SR0		0xf0000000	/* Saved bits from CR register */
#define MSR_KEY		BIT(12)		/* Copy of Ks or Kp bit */
#define MSR_IMISS	BIT(13)		/* It was an I miss */
#define MSR_WAY		BIT(14)		/* TLB set to be replaced */
#define MSR_STORE	BIT(15)		/* Miss caused by a store */

/*
 * Exception codes (trap vectors)
 */
#define CRESET	0x01
#define CMCHECK	0x02
#define CDSI		0x03
#define CISI		0x04
#define CEI		0x05
#define CALIGN		0x06
#define CPROG		0x07
#define CFPU		0x08
#define CDEC		0x09
#define CSYSCALL	0x0C
#define CTRACE		0x0D	/* optional */
#define CFPA		0x0E	/* not implemented in 603e */

/* PPC603e-specific: */
#define CIMISS		0x10	/* Instruction TLB miss */
#define CLMISS		0x11	/* Data load TLB miss */
#define CSMISS		0x12	/* Data store TLB miss */
#define CIBREAK		0x13
#define CSMI		0x14

/*
 * Magic registers
 */

#define	MACH		30	/* R30 is m-> */
#define	USER		29	/* R29 is up-> */


/*
 *  virtual MMU
 */
#define PTEMAPMEM	(1024*1024)	
#define PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	1984
#define SSEGMAPSIZE	16
#define PPN(x)		((x)&~(BY2PG-1))

/*
 *  First pte word
 */
#define	PTE0(v, vsid, h, va)	(((v)<<31)|((vsid)<<7)|((h)<<6)|(((va)>>22)&0x3f))

/*
 *  Second pte word; WIMG & PP(RW/RO) common to page table and BATs
 */
#define	PTE1_R		BIT(23)
#define	PTE1_C		BIT(24)

#define	PTE1_W		BIT(25)
#define	PTE1_I		BIT(26)
#define	PTE1_M		BIT(27)
#define	PTE1_G		BIT(28)

#define	PTE1_RW		BIT(30)
#define	PTE1_RO		BIT(31)

/* HID0 register bits */
#define	HID_ICE		BIT(16)
#define	HID_DCE		BIT(17)
#define	HID_ILOCK	BIT(18)
#define	HID_DLOCK	BIT(19)
#define	HID_ICFI	BIT(20)
#define	HID_DCFI	BIT(21)
#define	HID_IFEM	BIT(24)

/*
 * Address spaces
 */

#define	KZERO		0x80000000		/* base of kernel address space */
#define	KTZERO		0x80100000		/* first address in kernel text */
#define	UZERO		0			/* base of user address space */
#define	UTZERO		(UZERO+BY2PG)		/* first address in user text */
#define UTROUND(t)	ROUNDUP((t), 0x100000)
#define	USTKTOP		(TSTKTOP-TSTKSIZ*BY2PG)	/* byte just beyond user stack */
#define	TSTKTOP		KZERO			/* top of temporary stack */
#define	TSTKSIZ		100
#define	USTKSIZE	(4*1024*1024)		/* size of user stack */
#define	UREGSIZE	((8+40)*4)
#define	MACHADDR	(KTZERO-MAXMACH*MACHSIZE)
#define	MACHPADDR	(MACHADDR&~KZERO)
#define	MACHP(n)	((Mach *)(MACHADDR+(n)*MACHSIZE))

#define isphys(x) (((ulong)x&KZERO)!=0)

/*
 * MPC8xx addresses
 */
#define	INTMEM		0xf0000000
#define	IOMEM		(INTMEM+0x10000)

#define getpgcolor(a)	0
