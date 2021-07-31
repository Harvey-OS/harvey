/*
 * arm exception handlers
 */
#include "arm.s"

#undef B					/* B is for 'botch' */

/*
 *  exception vectors, copied by trapinit() to somewhere useful
 */
TEXT vectors(SB), 1, $-4
	MOVW	0x18(R15), R15		/* reset */
	MOVW	0x18(R15), R15		/* undefined instr. */
	MOVW	0x18(R15), R15		/* SWI & SMC */
	MOVW	0x18(R15), R15		/* prefetch abort */
	MOVW	0x18(R15), R15		/* data abort */
	MOVW	0x18(R15), R15		/* reserved */
	MOVW	0x18(R15), R15		/* IRQ */
	MOVW	0x18(R15), R15		/* FIQ */

TEXT vtable(SB), 1, $-4
	WORD	$_vsvc(SB)		/* reset, in svc mode already */
	WORD	$_vund(SB)		/* undefined, switch to svc mode */
	WORD	$_vsvc(SB)		/* swi, in svc mode already */
	WORD	$_vpabt(SB)		/* prefetch abort, switch to svc mode */
	WORD	$_vdabt(SB)		/* data abort, switch to svc mode */
	WORD	$_vsvc(SB)		/* reserved */
	WORD	$_virq(SB)		/* IRQ, switch to svc mode */
	WORD	$_vfiq(SB)		/* FIQ, switch to svc mode */

TEXT _vrst(SB), 1, $-4
	BL	_reset(SB)

TEXT _vsvc(SB), 1, $-4			/* SWI */
	MOVW.W	R14, -4(R13)		/* ureg->pc = interrupted PC */
	MOVW	SPSR, R14		/* ureg->psr = SPSR */
	MOVW.W	R14, -4(R13)		/* ... */
	MOVW	$PsrMsvc, R14		/* ureg->type = PsrMsvc */
	MOVW.W	R14, -4(R13)		/* ... */

//	MOVM.DB.W.S [R0-R14], (R13)	/* save user level registers, at end r13 points to ureg */
	MOVM.DB.S [R0-R14], (R13)	/* save user level registers */
	SUB	$(15*4), R13		/* r13 now points to ureg */

	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */

	MOVW	$(L1-MACHSIZE), R10	/* m */
	MOVW	8(R10), R9		/* up */

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

TEXT _vund(SB), 1, $-4			/* undefined */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMund, R0
	B	_vswitch

TEXT _vpabt(SB), 1, $-4			/* prefetch abort */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMabt, R0		/* r0 = type */
	B	_vswitch

TEXT _vdabt(SB), 1, $-4			/* data abort */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$(PsrMabt+1), R0	/* r0 = type */
	B	_vswitch

TEXT _virq(SB), 1, $-4			/* IRQ */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMirq, R0		/* r0 = type */
	B	_vswitch

	/*
	 *  come here with type in R0 and R13 pointing above saved [r0-r4].
	 *  we'll switch to SVC mode and then call trap.
	 */
_vswitch:
	MOVW	SPSR, R1		/* save SPSR for ureg */
	MOVW	R14, R2			/* save interrupted pc for ureg */
	MOVW	R13, R3			/* save pointer to where the original [R0-R4] are */

	/*
	 * switch processor to svc mode.  this switches the banked registers
	 * (r13 [sp] and r14 [link]) to those of svc mode.
	 */
	MOVW	CPSR, R14
	BIC	$PsrMask, R14
	ORR	$(PsrDirq|PsrDfiq|PsrMsvc), R14
	MOVW	R14, CPSR		/* switch! */

	AND.S	$0xf, R1, R4		/* interrupted code kernel or user? */
	BEQ	_userexcep

	/* here for trap from SVC mode */
	MOVM.DB.W [R0-R2], (R13)	/* set ureg->{type, psr, pc}; r13 points to ureg->type  */
	MOVM.IA	  (R3), [R0-R4]		/* restore [R0-R4] from previous mode's stack */

	/*
	 * In order to get a predictable value in R13 after the stores,
	 * separate the store-multiple from the stack-pointer adjustment.
	 * We'll assume that the old value of R13 should be stored on the stack.
	 */
	/* save kernel level registers, at end r13 points to ureg */
//	MOVM.DB.W [R0-R14], (R13)
	MOVM.DB	[R0-R14], (R13)
	SUB	$(15*4), R13		/* SP now points to saved R0 */

	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$(4*2), R13		/* space for argument+link (for debugger) */
	MOVW	$0xdeaddead, R11	/* marker */

	BL	trap(SB)

	ADD	$(4*2+4*15), R13	/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */

	MOVM.DB (R13), [R0-R14]		/* restore registers */

	ADD	$(4*2), R13		/* pop past ureg->{type+psr} to pc */
	RFE				/* MOVM.IA.S.W (R13), [R15] */

	/* here for trap from USER mode */
_userexcep:
	MOVM.DB.W [R0-R2], (R13)	/* set ureg->{type, psr, pc}; r13 points to ureg->type  */
	MOVM.IA	  (R3), [R0-R4]		/* restore [R0-R4] from previous mode's stack */

//	MOVM.DB.W.S [R0-R14], (R13)	/* save kernel level registers, at end r13 points to ureg */
	MOVM.DB.S [R0-R14], (R13)	/* save kernel level registers */
	SUB	$(15*4), R13		/* r13 now points to ureg */

	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */

	MOVW	$(L1-MACHSIZE), R10	/* m */
	MOVW	8(R10), R9		/* up */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$(4*2), R13		/* space for argument+link (for debugger) */

	BL	trap(SB)

	ADD	$(4*2+4*15), R13	/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */
	MOVM.DB.S (R13), [R0-R14]	/* restore registers */
	ADD	$(4*2), R13		/* pop past ureg->{type+psr} */
	RFE				/* MOVM.IA.S.W (R13), [R15] */

TEXT _vfiq(SB), 1, $-4			/* FIQ */
WAVE('%')
	RFE				/* FIQ is special, ignore it for now */

/*
 *  set the stack value for the mode passed in R0
 */
TEXT setr13(SB), 1, $-4
	MOVW	4(FP), R1

	MOVW	CPSR, R2
	BIC	$PsrMask, R2, R3
	ORR	R0, R3
	MOVW	R3, CPSR		/* switch to new mode */

	MOVW	R13, R0			/* return old sp */
	MOVW	R1, R13			/* install new one */

	MOVW	R2, CPSR		/* switch back to old mode */
	RET
