/*
 * arm-specific definitions for cortex-a9
 * these are used in C and assembler
 *
 * unqualified `cortex' refers to the cortex-a8 or -a9.
 */

#define NREGS		15	/* general-purpose regs, R0 through R14 */

/*
 * Program Status Registers
 */
#define PsrMusr		0x00000010		/* mode */
#define PsrMfiq		0x00000011
#define PsrMirq		0x00000012
#define PsrMsvc		0x00000013	/* `protected mode for OS' */
#define PsrMmon		0x00000016	/* `secure monitor' (trustzone hyper) */
#define PsrMabt		0x00000017
#define PsrMund		0x0000001B
#define PsrMsys		0x0000001F	/* `privileged user mode for OS' (trustzone) */
#define PsrMask		0x0000001F

#define PsrThumb	0x00000020		/* beware hammers */
#define PsrDfiq		0x00000040		/* disable FIQ interrupts */
#define PsrDirq		0x00000080		/* disable IRQ interrupts */
#define PsrDasabt	0x00000100		/* disable asynch aborts */
#define PsrBigend	0x00000200		/* E: big-endian data */

#define PsrGe		0x000f0000		/* `>=' bits */
#define PsrMbz20	0x00f00000		/* MBZ: 20 - 23 */
#define PsrJaz		0x01000000		/* java mode */
#define PsrIT		0x0600fc00		/* IT: if-then thumb state */
#define PsrQ		0x08000000		/* cumulative saturation */

#define PsrV		0x10000000		/* overflow */
#define PsrC		0x20000000		/* carry/borrow/extend */
#define PsrZ		0x40000000		/* zero */
#define PsrN		0x80000000		/* negative/less than */

/* these bits must be 0 */
#define PsrMbz		(PsrQ|PsrJaz|PsrMbz20|PsrThumb|PsrBigend|PsrDasabt)

/*
 * MCR and MRC are anti-mnemonic.
 *	MTCP	coproc, opcode1, Rd, CRn, CRm[, opcode2]	# arm -> coproc
 *	MFCP	coproc, opcode1, Rd, CRn, CRm[, opcode2]	# coproc -> arm
 */

#define MTCP	MCR
#define MFCP	MRC

/*
 * instruction decoding
 */

#define CONDITION(inst)	((ulong)(inst) >> 28)
#define OPCODE(inst)	(((inst)>>24) & MASK(4))	/* co-proc opcode */
#define CPUREGN(inst)	(((inst)>>16) & MASK(4))	/* Rn */
#define CPUREGD(inst)	(((inst)>>12) & MASK(4))	/* Rd */
#define COPROC(inst)	(((inst)>> 8) & MASK(4))	/* co-proc number */

/* for double-word moves, use VFPREGD for Rt, VFPREGN for Rt2 */
#define VFPREGN(inst)	(((inst)>>16) & MASK(4))	/* Vn */
#define VFPREGD(inst)	(((inst)>>12) & MASK(4))	/* Vd */

/* op must be OPCODE(inst) */
#define ISCPOP(op)	((op) == 0xE || ((op) & ~1) == 0xC)
/* assume ISCPOP() is true; cp must be COPROC(inst) */
#define ISFPOP(cp)	((cp) == CpOFPA || (cp) == CpDFP || (cp) == CpFP)

/*
 * Coprocessors
 */
#define CpOFPA		1			/* ancient 7500 FPA */
#define CpFP		10			/* float VFP and config */
#define CpDFP		11			/* double VFP */
#define CpSC		15			/* System Control */

/*
 * Primary (CRn) CpSC registers.
 */
#define	CpID		0			/* ID and cache type */
#define	CpCONTROL	1			/* miscellaneous control */
#define	CpTTB		2			/* Translation Table Base(s) */
#define	CpDAC		3			/* Domain Access Control */
#define	CpFSR		5			/* Fault Status */
#define	CpFAR		6			/* Fault Address */
#define	CpCACHE		7			/* cache/write buffer control */
#define	CpTLB		8			/* TLB control */
#define	CpCLD		9			/* L2 Cache Lockdown, op1==1 */
#define CpTLD		10			/* TLB Lockdown, with op2 */
#define CpVECS		12			/* vector bases, op1==0, Crm==0, op2s (cortex) */
#define	CpPID		13			/* Process ID */
#define CpDTLB		15			/* TLB, L1 cache stuff (cortex) */

/*
 * CpTTB op1==0, Crm==0 opcode2 values.
 */
#define CpTTB0		0			/* secure ttb */
#define CpTTB1		1			/* non-secure ttb (v7) */
#define CpTTBctl	2			/* v7 */

/*
 * CpTTB[01] low-order bits (for MP)
 */
#define TTBIRGNwb	(1<<6)			/* inner write-back */
#define TTBNos		(1<<5)			/* inner shareable */
#define TTBRGNnowralloc	(1<<4)			/* outer no write alloc */
#define TTBRGNwb	(1<<3)			/* outer write-back */
#define TTBShare	(1<<1)			/* shareable */
#define TTBIRGNnowralloc (1<<0)			/* inner no write alloc */

/* we want normal, shareable, write-back, write-allocate cacheable memory */
#define TTBlow		(TTBIRGNwb|TTBRGNwb|TTBShare|TTBNos)

/*
 * CpFSR op1==0, Crm==0 opcode 2 values.
 */
#define CpDFSR		0			/* data fault status */
#define CpIFSR		1			/* instruction fault status */

/*
 * CpFAR op1==0, Crm==0 opcode 2 values.
 */
#define CpDFAR		0			/* data fault address */
#define CpIFAR		2			/* instruction fault address */

/*
 * CpID Secondary (CRm) registers.
 */
#define CpIDidct	0
#define CpIDidmmfr	1			/* memory model features */

/*
 * CpID CpIDidct op1==0 opcode2 fields.
 */
#define CpIDid		0			/* main ID */
#define CpIDct		1			/* cache type */
#define CpIDtlb		3			/* tlb type (cortex) */
#define CpIDmpid	5			/* multiprocessor id (cortex) */

/* CpIDid op1 values */
#define CpIDcsize	1			/* cache size (cortex) */
#define CpIDcssel	2			/* cache size select (cortex) */

/*
 * CpID CpIDidct op1==CpIDcsize opcode2 fields.
 */
#define CpIDcasize	0			/* cache size */
#define CpIDclvlid	1			/* cache-level id */

/*
 * CpID CpIDidmmfr op1==0 opcode 2 fields.
 */
#define CpIDidmmfr0	4			/* base of 4 regs */

/*
 * CpCONTROL op2 codes, op1==0, Crm==0.
 */
#define CpMainctl	0		/* sctlr */
#define CpAuxctl	1
#define CpCPaccess	2

/*
 * CpCONTROL: op1==0, CRm==0, op2==CpMainctl.
 * main control register.
 * cortex/armv7 has more ops and CRm values.
 */
#define CpCmmu		(1<<0)	/* M: MMU enable */
#define CpCalign	(1<<1)	/* A: alignment fault enable */
#define CpCdcache	(1<<2)	/* C: data and unified caches on */
#define CpC15ben	(1<<5)	/* cp15 barrier enable */
#define CpCbigend	(1<<7)	/* B: big-endian data */
#define CpCsw		(1<<10)	/* SW: SWP(B) enable (deprecated in v7) */
#define CpCpredict	(1<<11)	/* Z: branch prediction (armv7) */
#define CpCicache	(1<<12)	/* I: instruction cache on */
#define CpChv	(1<<13)		/* V: use high vectors at 0xffff0000 */
#define CpCrr	(1<<14)		/* RR: round robin vs random cache replacement */
#define CpCha	(1<<17)		/* HA: hw access flag enable */
#define CpCwxn	(1<<19)		/* WXN: write implies XN (VE) */
#define CpCuwxn	(1<<20) /* UWXN: force XN for unpriv. writeable regions (VE) */
#define CpCfi	(1<<21)		/* FI: fast intrs */
#define CpCve	(1<<24)		/* VE: intr vectors enable */
#define CpCee	(1<<25)		/* EE: exception endianness: big */
#define CpCnmfi	(1<<27)		/* NMFI: non-maskable fast intrs. (RO) */
#define CpCtre	(1<<28)		/* TRE: TEX remap enable */
#define CpCafe	(1<<29)		/* AFE: access flag (ttb) enable */
#define CpCte	(1<<30)		/* TE: thumb exceptions */

/* must-be-0 bits (armv7) */
#define CpCsbz (1<<31 | CpCte | CpCafe | CpCtre | 1<<26 | CpCee | CpCve | \
	CpCfi | CpCuwxn | CpCwxn | CpCha | 1<<15 | 3<<8 | CpCbigend)
/* must be 1 (armv7) */
/* CpCrr is for erratum 716044 */
#define CpCsbo (3<<22 | 1<<18 | 1<<16 | CpCrr | CpCsw | 1<<6 | \
	CpC15ben | 3<<3)

/*
 * CpCONTROL: op1==0, CRm==0, op2==CpAuxctl.
 * Auxiliary control register on cortex-a9.
 * these differ from even the cortex-a8 bits.
 */
#define CpACparity	(1<<9)
#define CpACca1way	(1<<8)	/* cache in a single way */
#define CpACcaexcl	(1<<7)	/* exclusive cache */
#define CpACsmpcoher	(1<<6)	/* SMP: scu keeps caches coherent; */
				/* needed for ldrex/strex */
#define CpAClwr0line	(1<<3)	/* write full cache line of 0s; see Fullline0 */
#define CpACl1pref	(1<<2)	/* l1 prefetch enable */
#define CpACl2pref	(1<<1)	/* l2 prefetch enable */
#define CpACmaintbcast	(1<<0)	/* FW: broadcast cache & tlb maint. ops */

/*
 * CpCONTROL Secondary (CRm) registers and opcode2 fields.
 */
#define CpCONTROLscr	1

#define CpSCRscr	0		/* secure configuration */
#define CpSCRdebug	1		/* undocumented errata workaround */

/*
 * CpCACHE Secondary (CRm) registers and opcode2 fields.  op1==0.
 * In ARM-speak, 'flush' means invalidate and 'clean' means writeback.
 */
//#define CpCACHEintr	0		/* interrupt (op2==4) */
#define CpCACHEinviis	1		/* inv i-cache to inner-sharable (v7) */
//#define CpCACHEpaddr	4		/* 0: phys. addr (cortex) */
#define CpCACHEinvi	5		/* inv i-cache to pou, branch table */
#define CpCACHEinvd	6		/* inv data or unified to poc */
// #define CpCACHEinvu	7		/* unified (not on cortex) */
#define CpCACHEva2pa	8		/* va -> pa translation (cortex) */
#define CpCACHEwb	10		/* writeback to poc */
#define CpCACHEwbsepou	11		/* wb data or unified by mva to pou */
#define CpCACHEwbi	14		/* writeback+invalidate to poc */

#define CpCACHEall	0		/* entire (not for invd nor wb(i) on cortex) */
#define CpCACHEse	1		/* single entry by va */
#define CpCACHEsi	2		/* set/index (set/way) */
#define CpCACHEinvuis	3		/* inv. u. tlb to is */
#define CpCACHEwait	4		/* wait (prefetch flush on cortex) */
//#define CpCACHEdmbarr	5		/* wb only (cortex) */
#define CpCACHEflushbtc 6		/* flush branch-target cache (cortex) */
#define CpCACHEflushbtse 7		/* ⋯ or just one entry in it (cortex) */

/*
 * CpTLB op1==0 Secondary (CRm) registers and opcode2 fields
 */
#define CpTLBinvuis	3		/* unified on all inner sharable cpus */
#define CpTLBinvi	5		/* instruction (deprecated) */
#define CpTLBinvd	6		/* data (deprecated) */
#define CpTLBinvu	7		/* unified */

#define CpTLBinv	0		/* invalidate all */
#define CpTLBinvse	1		/* invalidate by mva */
#define CpTLBasid	2		/* by ASID (cortex) */
#define CpTLBallasid	3		/* by mva, all ASIDs (cortex) */

/*
 * CpCLD Secondary (CRm) registers and opcode2 fields for op1==0. (cortex)
 */
#define CpCLDena	12		/* enables */
#define CpCLDcyc	13		/* cycle counter */
#define CpCLDuser	14		/* user enable */

#define CpCLDenapmnc	0
#define CpCLDenacyc	1

/*
 * CpCLD Secondary (CRm) registers and opcode2 fields for op1==1.
 */
#define CpCLDl2		0		/* l2 cache */

#define CpCLDl2aux	2		/* auxiliary control */

/*
 * l2 cache aux. control
 */
#define CpCl2ecc	(1<<28)			/* use ecc, not parity */
#define CpCl2noldforw	(1<<27)			/* no ld forwarding */
#define CpCl2nowrcomb	(1<<25)			/* no write combining */
#define CpCl2nowralldel	(1<<24)			/* no write allocate delay */
#define CpCl2nowrallcomb (1<<23)		/* no write allocate combine */
#define CpCl2nowralloc	(1<<22)			/* no write allocate */
#define CpCl2eccparity	(1<<21)			/* enable ecc or parity */
#define CpCl2inner	(1<<16)			/* inner cacheability */
/* other bits are tag ram & data ram latencies */

/*
 * CpTLD Secondary (CRm) registers and opcode2 fields.
 */
#define CpTLDlock	0			/* TLB lockdown registers */
#define CpTLDpreload	1			/* TLB preload */

#define CpTLDi		0			/* TLB instr. lockdown reg. */
#define CpTLDd		1			/* " data " " */

/*
 * CpVECS Secondary (CRm) registers and opcode2 fields.
 */
#define CpVECSbase	0

#define CpVECSnorm	0			/* (non-)secure base addr */
#define CpVECSmon	1			/* secure monitor base addr */

/*
 * CpDTLB Secondary (CRm) registers and opcode 1 and 2 fields.
 */
#define CpDTLBmisc	0

/* op1 */
#define CpDTLBmisc1	0
#define CpDTLBcbar1	4	/* config base address (periphbase for scu) */

/* op2 for op1==0 (CpDTLBmisc1) */
#define CpDTLBpower	0	/* power control */
#define CpDTLBdiag	1	/* `undocumented diagnostic register' */

/* op2 for op1==CpDTLBcbar1 */
#define CpDTLBcbar2	0	/* config base address (periphbase for scu) */

/*
 * MMU page table entries.
 * memory must be cached, buffered, sharable and wralloc to participate in
 * automatic L1 cache coherency.
 *
 * the Noexec* bits prevent prefetching, which is otherwise speculative and
 * aggressive in the cortex-a9, particularly translation table walks.
 * this property is documented in the (vast, legalistic) v7-a arch. arm. in
 * §B3.7.2, but I first saw it discussed by Adeneo Embedded (and it fixed
 * many crashes for them) in https://community.nxp.com/docs/DOC-103079.
 */
#define L1pagesmbz	(0<<4)			/* L1 page tables: must be 0 */
#define Noexecsect	(1<<4)			/* L1 sections: no execute */
#define Fault		0x00000000		/* L[12] pte: unmapped */

#define Coarse		(L1pagesmbz|1)		/* L1: page table */
#define Section		(L1pagesmbz|2)		/* L1 1MB */
/*
 * next 2 bits (L1wralloc & L1sharable) and Buffered and Cached must be
 * set in l1 ptes for LDREX/STREX to work.
 */
#define L1wralloc	(1<<12)			/* L1 TEX */
#define L1sharable	(1<<16)
#define L1nonglobal	(1<<17)			/* tied to asid */
#define Nonsecuresect	(1<<19)			/* L1 sections */

#define Large		0x00000001		/* L2 64KB */
#define Noexecsmall	1			/* L2 4KB page: no execute */
#define Small		0x00000002		/* L2 4KB */
/*
 * next 3 bits (Buffered, Cached, L2wralloc) & L2sharable must be set in
 * l2 ptes for memory containing locks because LDREX/STREX require them.
 */
#define Buffered	0x00000004		/* L[12]: 0 write-thru, 1 -back */
#define Cached		0x00000008		/* L[12] */
#define L2wralloc	(1<<6)			/* L2 TEX (small pages) */
#define L2apro		(1<<9)			/* L2 AP: read only */
#define L2sharable	(1<<10)
#define L2nonglobal	(1<<11)			/* tied to asid */
#define Dom0		0

/* attributes for memory containing locks */
#define L1ptedramattrs	(Cached | Buffered | L1wralloc | L1sharable)
#define L2ptedramattrs	(Cached | Buffered | L2wralloc | L2sharable)

#define Noaccess	0			/* AP, DAC */
#define Krw		1			/* AP */
/* armv7 deprecates AP[2] == 1 & AP[1:0] == 2 (Uro), prefers 3 (new in v7) */
#define Uro		2			/* AP */
#define Urw		3			/* AP */
#define Client		1			/* DAC */
#define Manager		3			/* DAC */

#define AP(n, v)	F((v), ((n)*2)+4, 2)
#define L1AP(ap)	(AP(3, (ap)))
#define L2AP(ap)	(AP(0, (ap)))		/* armv7 */
#define DAC(n, v)	F((v), (n)*2, 2)
