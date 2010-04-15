/*
 * Program Status Registers
 */
#define PsrMusr		0x00000010		/* mode */
#define PsrMfiq		0x00000011
#define PsrMirq		0x00000012
#define PsrMsvc		0x00000013
#define PsrMabt		0x00000017
#define PsrMund		0x0000001B
#define PsrMsys		0x0000001F
#define PsrMask		0x0000001F

#define PsrDfiq		0x00000040		/* disable FIQ interrupts */
#define PsrDirq		0x00000080		/* disable IRQ interrupts */

#define PsrV		0x10000000		/* overflow */
#define PsrC		0x20000000		/* carry/borrow/extend */
#define PsrZ		0x40000000		/* zero */
#define PsrN		0x80000000		/* negative/less than */

/*
 * Coprocessors
 */
#define CpFP		10			/* float FP, VFP cfg. */
#define CpDFP		11			/* double FP */
#define CpSC		15			/* System Control */

/*
 * opcode 1
 */
#define	CpDef		0			/* default */
#define CpL2		1			/* L2 cache operations */

/*
 * Primary (CRn) CpSC registers.
 */
#define	CpID		0			/* ID and cache type */
#define	CpCONTROL	1			/* miscellaneous control */
#define	CpTTB		2			/* Translation Table Base */
#define	CpDAC		3			/* Domain Access Control */
#define	CpFSR		5			/* Fault Status */
#define	CpFAR		6			/* Fault Address */
#define	CpCACHE		7			/* cache/write buffer control */
#define	CpTLB		8			/* TLB control */
#define	CpCLD		9			/* Cache Lockdown */
#define CpTLD		10			/* TLB Lockdown */
#define	CpPID		13			/* Process ID */
#define CpTESTCFG	15			/* test config. (arm926) */

/*
 * CpID Secondary (CRm) registers.
 */
#define CpIDidct	0
/*
 * CpID op1==0 opcode2 fields.
 */
#define CpIDid		0			/* main ID */
#define CpIDct		1			/* cache type */

/*
 * CpCONTROL
 */
#define CpCmmu		0x00000001		/* M: MMU enable */
#define CpCalign	0x00000002		/* A: alignment fault enable */
#define CpCdcache	0x00000004		/* C: data cache on */
#define CpCwb		0x00000008		/* W: write buffer turned on */
#define CpCi32		0x00000010		/* P: 32-bit program space */
#define CpCd32		0x00000020		/* D: 32-bit data space */
#define CpCbe		0x00000080		/* B: big-endian operation */
#define CpCsystem	0x00000100		/* S: system permission */
#define CpCrom		0x00000200		/* R: ROM permission */
#define CpCicache	0x00001000		/* I: instruction cache on */
#define CpChv		0x00002000		/* V: high vectors */

/*
 * CpCACHE Secondary (CRm) registers and opcode2 fields.
 * In ARM-speak, 'flush' means invalidate and 'clean' means writeback.
 * In arm arch v6, these must be available in user mode:
 *	CpCACHEinvi, CpCACHEwait (prefetch flush)
 *	CpCACHEwb, CpCACHEwait (DSB: data sync barrier)
 *	CpCACHEwb, CpCACHEdmbarr (DMB: data memory barrier)
 */
#define CpCACHEintr	0			/* interrupt */
#define CpCACHEinvi	5			/* instruction */
#define CpCACHEinvd	6			/* data */
#define CpCACHEinvu	7			/* unified (I+D) */
#define CpCACHEwb	10			/* writeback D */
#define CpCACHEwbu	11			/* writeback U (not 926ejs) */
#define CpCACHEwbi	14			/* writeback D + invalidate */
#define CpCACHEwbui	15			/* writeback U + inval (not 926ejs) */

/*
 * the 926ejs manual says that we can't use CpCACHEall nor CpCACHEwait
 * for writeback operations on the 926ejs, except for CpCACHEwb + CpCACHEwait,
 * which means `drain write buffer'.
 */
#define CpCACHEall	0			/* entire */
#define CpCACHEse	1			/* single entry */
#define CpCACHEsi	2			/* set/index */
#define CpCACHEtest	3			/* test loop */
#define CpCACHEwait	4			/* wait */
#define CpCACHEdmbarr	5			/* wb: data memory barrier */

/*
 * CpTLB Secondary (CRm) registers and opcode2 fields.
 */
#define CpTLBinvi	5			/* instruction */
#define CpTLBinvd	6			/* data */
#define CpTLBinvu	7			/* unified */

#define CpTLBinv	0			/* invalidate all */
#define CpTLBinvse	1			/* invalidate single entry */

/*
 * CpTESTCFG Secondary (CRm) registers and opcode2 fields; sheeva only.
 * opcode1 == CpL2 (1).  L2 cache operations block the CPU until finished.
 * Specifically, write-back (clean) blocks until all dirty lines have been
 * drained from the L2 buffers.
 */
#define CpTCl2cfg	1
#define CpTCl2flush	9			/* cpu blocks until flush done */
#define CpTCl2waylck	10
#define CpTCl2inv	11
#define CpTCl2perfctl	12
#define CpTCl2perfcnt	13

/* CpTCl2cfg */
#define CpTCl2conf	0

/* CpTCl2conf bits */
#define	CpTCldcstream	(1<<29)			/* D cache streaming switch */
#define	CpTCl2wralloc	(1<<28)			/* cache write allocate */
#define	CpTCl2prefdis	(1<<24)			/* l2 cache prefetch disable */
#define	CpTCl2ena	(1<<22)			/* l2 cache enable */

/* CpTCl2flush & CpTCl2inv */
#define CpTCl2all	0
#define CpTCl2seva	1
#define CpTCl2way	2
#define CpTCl2sepa	3
#define CpTCl2valow	4
#define CpTCl2vahigh	5			/* also triggers flush or inv */

/* CpTCl2flush
#define CpTCecccnt	6			/* ecc error count */
#define CpTCeccthr	7			/* ecc error threshold */

/* CpTCl2waylck */
#define CpTCl2waylock	7

/* CpTCl2inv */
#define CpTCl2erraddr	7			/* ecc error address */

/* CpTCl2perfctl */
#define CpTCl2perf0ctl	0
#define CpTCl2perf1ctl	1

/* CpTCl2perfcnt */
#define CpTCl2perf0low	0
#define CpTCl2perf0high	1
#define CpTCl2perf1low	2
#define CpTCl2perf1high	3

/*
 * MMU page table entries.
 * Mbo (0x10) bit is implementation-defined and mandatory on some pre-v7 arms.
 */
#define Mbo		0x10			/* must be 1 on earlier arms */
#define Fault		0x00000000		/* L[12] pte: unmapped */

#define Coarse		(Mbo|1)			/* L1 */
#define Section		(Mbo|2)			/* L1 1MB */
#define Fine		(Mbo|3)			/* L1 */

#define Large		0x00000001u		/* L2 64KB */
#define Small		0x00000002u		/* L2 4KB */
#define Tiny		0x00000003u		/* L2 1KB, deprecated */
#define Buffered	0x00000004u		/* L[12]: write-back not -thru */
#define Cached		0x00000008u		/* L[12] */

#define Dom0		0
#define Noaccess	0			/* AP, DAC */
#define Krw		1			/* AP */
#define Uro		2			/* AP */
#define Urw		3			/* AP */
#define Client		1			/* DAC */
#define Manager		3			/* DAC */

#define AP(n, v) F((v), ((n)*2)+4, 2)
#define L1AP(ap) (AP(3, (ap)))		/* in L1, only Sections have AP */
#define L2AP(ap) (AP(3, (ap))|AP(2, (ap))|AP(1, (ap))|AP(0, (ap))) /* pre-armv7 */
#define DAC(n, v) F((v), (n)*2, 2)

#define HVECTORS	0xffff0000		/* addr of vectors */
