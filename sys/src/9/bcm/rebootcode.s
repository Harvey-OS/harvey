/*
 * armv6/armv7 reboot code
 */
#include "arm.s"

#define PTEDRAM		(Dom0|L1AP(Krw)|Section)

#define WFI	WORD	$0xe320f003	/* wait for interrupt */
#define WFE	WORD	$0xe320f002	/* wait for event */

/*
 * CPU0:
 *   main(PADDR(entry), PADDR(code), size);
 * Copy the new kernel to its correct location in virtual memory.
 * Then turn off the mmu and jump to the start of the kernel.
 *
 * Other CPUs:
 *   main(0, soc.armlocal, 0);
 * Turn off the mmu, wait for a restart address from CPU0, and jump to it.
 */

/* */
TEXT	main(SB), 1, $-4
	MOVW	$setR12(SB), R12

	/* copy in arguments before stack gets unmapped */
	MOVW	R0, R8			/* entry point */
	MOVW	p2+4(FP), R7		/* source */
	MOVW	n+8(FP), R6		/* byte count */

	/* redo double map of first MiB PHYSDRAM = KZERO */
	MOVW	12(R(MACH)), R2		/* m->mmul1 (virtual addr) */
	MOVW	$PTEDRAM, R1		/* PTE bits */
	MOVW	R1, (R2)
	DSB
	MCR	CpSC, 0, R2, C(CpCACHE), C(CpCACHEwb), CpCACHEse

	/* invalidate stale TLBs */
	BARRIERS
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/* relocate PC to physical addressing */
	MOVW	$_reloc(SB), R15

TEXT _reloc(SB), $-4
	
	/* continue with reboot only on cpu0 */
	CPUID(R2)
	BEQ	bootcpu

	/* other cpus wait for inter processor interrupt from cpu0 */

	/* turn caches off, invalidate icache */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
 	/* turn off mmu */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$CpCmmu, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	/* turn off SMP cache snooping */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BIC	$CpACsmp, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ISB
	DSB
	/* turn icache back on */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpCicache), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

dowfi:
	WFE			/* wait for event signal */
	MOVW	$0xCC(R7), R1	/* inter-core .startcpu mailboxes */
	ADD	R2<<4, R1	/* mailbox for this core */
	MOVW	0(R1), R8	/* content of mailbox */
	CMP	$0, R8		
	BEQ	dowfi		/* if zero, wait again */
	BL	(R8)		/* call received address */
	B	dowfi		/* shouldn't return */

bootcpu:
	MOVW	$PADDR(MACHADDR+MACHSIZE-4), SP

	/* copy the kernel to final destination */
	MOVW	R8, R9		/* save physical entry point */
	ADD	$KZERO, R8	/* relocate dest to virtual */
	ADD	$KZERO, R7	/* relocate src to virtual */
	ADD	$3, R6		/* round length to words */
	BIC	$3, R6
memloop:
	MOVM.IA.W	(R7), [R1]
	MOVM.IA.W	[R1], (R8)
	SUB.S	$4, R6
	BNE	memloop

	/* clean dcache using appropriate code for armv6 or armv7 */
	MRC	CpSC, 0, R1, C(CpID), C(CpIDfeat), 7	/* Memory Model Feature Register 3 */
	TST	$0xF, R1	/* hierarchical cache maintenance? */
	BNE	l2wb
	DSB
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEall
	B	l2wbx
l2wb:
	BL		cachedwb(SB)
	BL		l2cacheuwb(SB)
l2wbx:

	/* turn caches off, invalidate icache */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache|CpCpredict), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	DSB
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	DSB
 	/* turn off mmu */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$CpCmmu, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS
	/* turn off SMP cache snooping */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BIC	$CpACsmp, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl

	/* invalidate dcache using appropriate code for armv6 or armv7 */
	MRC	CpSC, 0, R1, C(CpID), C(CpIDfeat), 7	/* Memory Model Feature Register 3 */
	TST	$0xF, R1	/* hierarchical cache maintenance */
	BNE	l2inv
	DSB
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEall
	B	l2invx
l2inv:
	BL		cachedinv(SB)
	BL		l2cacheuinv(SB)
l2invx:

	/* jump to restart entry point */
	MOVW	R9, R8
	MOVW	$0, R9
	B	(R8)

#define ICACHELINESZ	32
#include "cache.v7.s"
