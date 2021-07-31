#include "mem.h"

/*
 * Entered here from Compaq's bootldr with MMU disabled.
 */
TEXT _start(SB), $-4
	MOVW	$setR12(SB), R12		/* load the SB */
_main:
	/* SVC mode, interrupts disabled */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	MOVW	R1, CPSR

	/* disable the MMU */
	MOVW	$0x130, R1
	MCR     CpMMU, 0, R1, C(CpControl), C(0x0)

	/* flush caches */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0x7), 0
	/* drain prefetch */
	MOVW	R0,R0						
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0

	/* drain write buffer */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 4

	MOVW	$(MACHADDR+BY2PG), R13		/* stack */
	SUB	$4, R13				/* link */
	BL	main(SB)
	BL	exit(SB)
	/* we shouldn't get here */
_mainloop:
	B	_mainloop
	BL	_div(SB)			/* hack to get _div etc loaded */

/* flush tlb's */
TEXT mmuinvalidate(SB), $-4
	MCR	CpMMU, 0, R0, C(CpTLBFlush), C(0x7)
	RET

/* flush tlb's */
TEXT mmuinvalidateaddr(SB), $-4
	MCR	CpMMU, 0, R0, C(CpTLBFlush), C(0x6), 1
	RET

/* write back and invalidate i and d caches */
TEXT cacheflush(SB), $-4
	/* write back any dirty data */
	MOVW	$0xe0000000,R0
	ADD	$(8*1024),R0,R1
_cfloop:
	MOVW.P	32(R0),R2
	CMP.S	R0,R1
	BNE	_cfloop
	
	/* drain write buffer and invalidate i&d cache contents */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 4
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0x7), 0

	/* drain prefetch */
	MOVW	R0,R0						
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0
	RET

/* write back d cache */
TEXT cachewb(SB), $-4
	/* write back any dirty data */
_cachewb:
	MOVW	$0xe0000000,R0
	ADD	$(8*1024),R0,R1
_cwbloop:
	MOVW.P	32(R0),R2
	CMP.S	R0,R1
	BNE	_cwbloop
	
	/* drain write buffer */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 4
	RET

/* write back a single cache line */
TEXT cachewbaddr(SB), $-4
	BIC	$31,R0
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 1
	B	_wbflush

/* write back a region of cache lines */
TEXT cachewbregion(SB), $-4
	MOVW	4(FP),R1
	CMP.S	$(4*1024),R1
	BGT	_cachewb
	ADD	R0,R1
	BIC	$31,R0
_cwbrloop:
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 1
	ADD	$32,R0
	CMP.S	R0,R1
	BGT	_cwbrloop
	B	_wbflush

/* invalidate the dcache */
TEXT dcacheinvalidate(SB), $-4
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0x6)
	RET

/* invalidate the icache */
TEXT icacheinvalidate(SB), $-4
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0x9)
	RET

/* drain write buffer */
TEXT wbflush(SB), $-4
_wbflush:
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 4
	RET

/* return cpu id */
TEXT getcpuid(SB), $-4
	MRC	CpMMU, 0, R0, C(CpCPUID), C(0x0)
	RET

/* return fault status */
TEXT getfsr(SB), $-4
	MRC	CpMMU, 0, R0, C(CpFSR), C(0x0)
	RET

/* return fault address */
TEXT getfar(SB), $-4
	MRC	CpMMU, 0, R0, C(CpFAR), C(0x0)
	RET

/* return fault address */
TEXT putfar(SB), $-4
	MRC	CpMMU, 0, R0, C(CpFAR), C(0x0)
	RET

/* set the translation table base */
TEXT putttb(SB), $-4
	MCR	CpMMU, 0, R0, C(CpTTB), C(0x0)
	RET

/*
 *  enable mmu, i and d caches
 */
TEXT mmuenable(SB), $-4
	MRC	CpMMU, 0, R0, C(CpControl), C(0x0)
	ORR	$(CpCmmuena|CpCdcache|CpCicache|CpCwb), R0
	MCR     CpMMU, 0, R0, C(CpControl), C(0x0)
	RET

TEXT mmudisable(SB), $-4
	MRC	CpMMU, 0, R0, C(CpControl), C(0x0)
	BIC	$(CpCmmuena|CpCdcache|CpCicache|CpCwb|CpCvivec), R0
	MCR     CpMMU, 0, R0, C(CpControl), C(0x0)
	RET

/*
 *  use exception vectors at 0xffff0000
 */
TEXT mappedIvecEnable(SB), $-4
	MRC	CpMMU, 0, R0, C(CpControl), C(0x0)
	ORR	$(CpCvivec), R0
	MCR     CpMMU, 0, R0, C(CpControl), C(0x0)
	RET
TEXT mappedIvecDisable(SB), $-4
	MRC	CpMMU, 0, R0, C(CpControl), C(0x0)
	BIC	$(CpCvivec), R0
	MCR     CpMMU, 0, R0, C(CpControl), C(0x0)
	RET

/* set the translation table base */
TEXT putdac(SB), $-4
	MCR	CpMMU, 0, R0, C(CpDAC), C(0x0)
	RET

/* set address translation pid */
TEXT putpid(SB), $-4
	MCR	CpMMU, 0, R0, C(CpPID), C(0x0)
	RET

/*
 *  set the stack value for the mode passed in R0
 */
TEXT setr13(SB), $-4
	MOVW	4(FP), R1

	MOVW	CPSR, R2
	BIC	$PsrMask, R2, R3
	ORR	R0, R3
	MOVW	R3, CPSR

	MOVW	R13, R0
	MOVW	R1, R13

	MOVW	R2, CPSR
	RET

/*
 *  exception vectors, copied by trapinit() to somewhere useful
 */

TEXT vectors(SB), $-4
	MOVW	0x18(R15), R15			/* reset */
	MOVW	0x18(R15), R15			/* undefined */
	MOVW	0x18(R15), R15			/* SWI */
	MOVW	0x18(R15), R15			/* prefetch abort */
	MOVW	0x18(R15), R15			/* data abort */
	MOVW	0x18(R15), R15			/* reserved */
	MOVW	0x18(R15), R15			/* IRQ */
	MOVW	0x18(R15), R15			/* FIQ */

TEXT vtable(SB), $-4
	WORD	$_vsvc(SB)			/* reset, in svc mode already */
	WORD	$_vund(SB)			/* undefined, switch to svc mode */
	WORD	$_vsvc(SB)			/* swi, in svc mode already */
	WORD	$_vpabt(SB)			/* prefetch abort, switch to svc mode */
	WORD	$_vdabt(SB)			/* data abort, switch to svc mode */
	WORD	$_vsvc(SB)			/* reserved */
	WORD	$_virq(SB)			/* IRQ, switch to svc mode */
	WORD	$_vfiq(SB)			/* FIQ, switch to svc mode */

TEXT _vrst(SB), $-4
	BL	resettrap(SB)

TEXT _vsvc(SB), $-4			/* SWI */
	MOVW.W	R14, -4(R13)		/* ureg->pc = interupted PC */
	MOVW	SPSR, R14		/* ureg->psr = SPSR */
	MOVW.W	R14, -4(R13)		/* ... */
	MOVW	$PsrMsvc, R14		/* ureg->type = PsrMsvc */
	MOVW.W	R14, -4(R13)		/* ... */
	MOVM.DB.W.S [R0-R14], (R13)	/* save user level registers, at end r13 points to ureg */
	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */
	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$8, R13			/* space for argument+link */

	BL	syscall(SB)

	ADD	$(8+4*15), R13		/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */
	MOVM.DB.S (R13), [R0-R14]	/* restore registers */
	ADD	$8, R13			/* pop past ureg->{type+psr} */
	RFE				/* MOVM.IA.S.W (R13), [R15] */

TEXT _vund(SB), $-4			/* undefined */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMund, R0
	B	_vswitch

TEXT _vpabt(SB), $-4			/* prefetch abort */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMabt, R0		/* r0 = type */
	B	_vswitch

TEXT _vdabt(SB), $-4			/* prefetch abort */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$(PsrMabt+1), R0		/* r0 = type */
	B	_vswitch

TEXT _virq(SB), $-4			/* IRQ */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMirq, R0		/* r0 = type */
	B	_vswitch

	/*
	 *  come here with type in R0 and R13 pointing above saved [r0-r4]
	 *  and type in r0.  we'll switch to SVC mode and then call trap.
	 */
_vswitch:
	MOVW	SPSR, R1		/* save SPSR for ureg */
	MOVW	R14, R2			/* save interrupted pc for ureg */
	MOVW	R13, R3			/* save pointer to where the original [R0-R3] are */

	/* switch to svc mode */
	MOVW	CPSR, R14
	BIC	$PsrMask, R14
	ORR	$(PsrDirq|PsrDfiq|PsrMsvc), R14
	MOVW	R14, CPSR

	/* interupted code kernel or user? */
	AND.S	$0xf, R1, R4
	BEQ	_userexcep

	/* here for trap from SVC mode */
	MOVM.DB.W [R0-R2], (R13)	/* set ureg->{type, psr, pc}; r13 points to ureg->type  */
	MOVM.IA	  (R3), [R0-R4]		/* restore [R0-R4] from previous mode's stack */
	MOVM.DB.W [R0-R14], (R13)	/* save kernel level registers, at end r13 points to ureg */
	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */
	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$8, R13			/* space for argument+link (for debugger) */
	MOVW	$0xdeaddead,R11		/* marker */

	BL	trap(SB)

	ADD	$(8+4*15), R13		/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */
	MOVM.DB (R13), [R0-R14]	/* restore registers */
	ADD	$8, R13			/* pop past ureg->{type+psr} */
	RFE				/* MOVM.IA.S.W (R13), [R15] */

	/* here for trap from USER mode */
_userexcep:
	MOVM.DB.W [R0-R2], (R13)	/* set ureg->{type, psr, pc}; r13 points to ureg->type  */
	MOVM.IA	  (R3), [R0-R4]		/* restore [R0-R4] from previous mode's stack */
	MOVM.DB.W.S [R0-R14], (R13)	/* save kernel level registers, at end r13 points to ureg */
	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */
	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$8, R13			/* space for argument+link (for debugger) */

	BL	trap(SB)

	ADD	$(8+4*15), R13		/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */
	MOVM.DB.S (R13), [R0-R14]	/* restore registers */
	ADD	$8, R13			/* pop past ureg->{type+psr} */
	RFE				/* MOVM.IA.S.W (R13), [R15] */

TEXT _vfiq(SB), $-4			/* FIQ */
	RFE				/* FIQ is special, ignore it for now */

/*
 *  This is the first jump from kernel to user mode.
 *  Fake a return from interrupt.
 *
 *  Enter with R0 containing the user stack pointer.
 *  UTZERO + 0x20 is always the entry point.
 *  
 */
TEXT touser(SB),$-4
	/* store the user stack pointer into the USR_r13 */
	MOVM.DB.W [R0], (R13)
	MOVM.S.IA.W (R13),[R13]

	/* set up a PSR for user level */
	MOVW	$(PsrMusr), R0
	MOVW	R0,SPSR

	/* save the PC on the stack */
	MOVW	$(UTZERO+0x20), R0
	MOVM.DB.W [R0],(R13)

	/* return from interrupt */
	RFE				/* MOVM.IA.S.W (R13), [R15] */
	
/*
 *  here to jump to a newly forked process
 */
TEXT forkret(SB),$-4
	ADD	$(4*15), R13		/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */
	MOVM.DB.S (R13), [R0-R14]	/* restore registers */
	ADD	$8, R13			/* pop past ureg->{type+psr} */
	RFE				/* MOVM.IA.S.W (R13), [R15] */

TEXT splhi(SB), $-4
	/* save caller pc in Mach */
	MOVW	$(MACHADDR+0x04),R2
	MOVW	R14,0(R2)
	/* turn off interrupts */
	MOVW	CPSR, R0
	ORR	$(PsrDfiq|PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT spllo(SB), $-4
	MOVW	CPSR, R0
	BIC	$(PsrDfiq|PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splx(SB), $-4
	/* save caller pc in Mach */
	MOVW	$(MACHADDR+0x04),R2
	MOVW	R14,0(R2)
	/* reset interrupt level */
	MOVW	R0, R1
	MOVW	CPSR, R0
	MOVW	R1, CPSR
	RET

TEXT splxpc(SB), $-4				/* for iunlock */
	MOVW	R0, R1
	MOVW	CPSR, R0
	MOVW	R1, CPSR
	RET

TEXT spldone(SB), $0
	RET

TEXT islo(SB), $-4
	MOVW	CPSR, R0
	AND	$(PsrDfiq|PsrDirq), R0
	EOR	$(PsrDfiq|PsrDirq), R0
	RET

TEXT cpsrr(SB), $-4
	MOVW	CPSR, R0
	RET

TEXT spsrr(SB), $-4
	MOVW	SPSR, R0
	RET

TEXT getcallerpc(SB), $-4
	MOVW	0(R13), R0
	RET

TEXT tas(SB), $-4
	MOVW	R0, R1
	MOVW	$0xDEADDEAD, R2
	SWPW	R2, (R1), R0
	RET

TEXT setlabel(SB), $-4
	MOVW	R13, 0(R0)			/* sp */
	MOVW	R14, 4(R0)			/* pc */
	MOVW	$0, R0
	RET

TEXT gotolabel(SB), $-4
	MOVW	0(R0), R13			/* sp */
	MOVW	4(R0), R14			/* pc */
	MOVW	$1, R0
	RET


/* The first MCR instruction of this function needs to be on a cache-line
 * boundary; to make this happen, it will be copied (in trap.c).
 *
 * Doze puts the machine into idle mode.  Any interrupt will get it out
 * at the next instruction (the RET, to be precise).
 */
TEXT _doze(SB), $-4
	MOVW	$UCDRAMZERO, R1
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0
	MCR     CpPWR, 0, R0, C(CpTest), C(0x2), 2
	MOVW	(R1), R0
	MCR     CpPWR, 0, R0, C(CpTest), C(0x8), 2
	RET
