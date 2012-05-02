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
	MOVW	0x18(R15), R15		/* hypervisor call */
	MOVW	0x18(R15), R15		/* IRQ */
	MOVW	0x18(R15), R15		/* FIQ */

TEXT vtable(SB), 1, $-4
	WORD	$_vrst-KZERO(SB)	/* reset, in svc mode already */
	WORD	$_vund(SB)		/* undefined, switch to svc mode */
	WORD	$_vsvc(SB)		/* swi, in svc mode already */
	WORD	$_vpabt(SB)		/* prefetch abort, switch to svc mode */
	WORD	$_vdabt(SB)		/* data abort, switch to svc mode */
	WORD	$_vhype(SB)		/* hypervisor call */
	WORD	$_virq(SB)		/* IRQ, switch to svc mode */
	WORD	$_vfiq(SB)		/* FIQ, switch to svc mode */

/*
 * reset - start additional cpus
 */
TEXT _vrst(SB), 1, $-4
	/* running in the zero segment (pc is lower 256MB) */
	CPSMODE(PsrMsvc)		/* should be redundant */
	CPSID
	CPSAE
	SETEND(0)			/* force little-endian */
	BARRIERS
	SETZSB
	MOVW	$PsrMsvc, SPSR
	MOVW	$0, R14

	/* invalidate i-cache and branch-target cache */
	MTCP	CpSC, 0, PC, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	BARRIERS

	BL	cpureset(SB)
spin:
	B	spin

/*
 * system call
 */
TEXT _vsvc(SB), 1, $-4			/* SWI */
	CLREX
	BARRIERS
	/* stack is m->stack */
	MOVW.W	R14, -4(R13)		/* ureg->pc = interrupted PC */
	MOVW	SPSR, R14		/* ureg->psr = SPSR */
	MOVW.W	R14, -4(R13)		/* ... */
	MOVW	$PsrMsvc, R14		/* ureg->type = PsrMsvc */
	MOVW.W	R14, -4(R13)		/* ... */

	/* avoid the ambiguity described in notes/movm.w. */
	MOVM.DB.S [R0-R14], (R13)	/* save user level registers */
	SUB	$(NREGS*4), R13		/* r13 now points to ureg */

	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */

	/*
	 * set up m and up registers since user registers could contain anything
	 */
	CPUID(R1)
	SLL	$2, R1			/* convert to word index */
	MOVW	$machaddr(SB), R2
	ADD	R1, R2
	MOVW	(R2), R(MACH)		/* m = machaddr[cpuid] */
	CMP	$0, R(MACH)
	MOVW.EQ	$MACHADDR, R0		/* paranoia: use MACHADDR if 0 */
	MOVW	8(R(MACH)), R(USER)	/* up = m->proc */

	MOVW	((NREGS+1)*4)(R13), R2	/* saved SPSR (user mode) */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$8, R13			/* space for argument+link */

	BL	syscall(SB)
	/*
	 * caller saves on plan 9, so registers other than 9, 10, 13 & 14
	 * may have been trashed when we get here.
	 */

	MOVW	$setR12(SB), R12	/* reload kernel's SB */

	ADD	$(8+4*NREGS), R13	/* make r13 point to ureg->type */

	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
/*
 * return from user-mode exception.
 * expects new SPSR in R0.  R13 must point to ureg->type.
 */
_rfue:
TEXT rfue(SB), 1, $-4
	MOVW	R0, SPSR		/* ... */

	/*
	 * order on stack is type, psr, pc, but RFEV7 needs pc, psr.
	 * step on type and previous word to hold temporary values.
	 * we could instead change the order in which psr & pc are pushed.
	 */
	MOVW	4(R13), R1		/* psr */
	MOVW	8(R13), R2		/* pc */
	MOVW	R2, 4(R13)		/* pc */
	MOVW	R1, 8(R13)		/* psr */

	MOVM.DB.S (R13), [R0-R14]	/* restore user registers */
	ADD	$4, R13			/* pop type, sp -> pc */
	RFEV7W(13)


TEXT _vund(SB), 1, $-4			/* undefined */
	/* sp is m->sund */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMund, R0
	B	_vswitch

TEXT _vpabt(SB), 1, $-4			/* prefetch abort */
	/* sp is m->sabt */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMabt, R0		/* r0 = type */
	B	_vswitch

TEXT _vdabt(SB), 1, $-4			/* data abort */
	/* sp is m->sabt */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$(PsrMabt+1), R0	/* r0 = type */
	B	_vswitch

TEXT _virq(SB), 1, $-4			/* IRQ */
	/* sp is m->sirq */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMirq, R0		/* r0 = type */
	B	_vswitch

	/*
	 *  come here with type in R0 and R13 pointing above saved [r0-r4].
	 *  we'll switch to SVC mode and then call trap.
	 */
_vswitch:
// TEXT _vswtch(SB), 1, $-4		/* make symbol visible to debuggers */
	CLREX
	BARRIERS
	MOVW	SPSR, R1		/* save SPSR for ureg */
	/*
	 * R12 needs to be set before using PsrMbz, so BIGENDCHECK code has
	 * been moved below.
	 */
	MOVW	R14, R2			/* save interrupted pc for ureg */
	MOVW	R13, R3			/* save pointer to where the original [R0-R4] are */

	/*
	 * switch processor to svc mode.  this switches the banked registers
	 * (r13 [sp] and r14 [link]) to those of svc mode (so we must be sure
	 * to never get here already in svc mode).
	 */
	CPSMODE(PsrMsvc)		/* switch! */
	CPSID

	AND.S	$0xf, R1, R4		/* interrupted code kernel or user? */
	BEQ	_userexcep

	/*
	 * here for trap from SVC mode
	 */

	/* push ureg->{type, psr, pc} onto Msvc stack.
	 * r13 points to ureg->type after.
	 */
	MOVM.DB.W [R0-R2], (R13)
	MOVM.IA	  (R3), [R0-R4]		/* restore [R0-R4] from previous mode's stack */

	/*
	 * avoid the ambiguity described in notes/movm.w.
	 * In order to get a predictable value in R13 after the stores,
	 * separate the store-multiple from the stack-pointer adjustment.
	 * We'll assume that the old value of R13 should be stored on the stack.
	 */
	/* save kernel level registers, at end r13 points to ureg */
	MOVM.DB	[R0-R14], (R13)
	SUB	$(NREGS*4), R13		/* SP now points to saved R0 */

	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */
	/* previous mode was svc, so the saved spsr should be sane. */
	MOVW	((NREGS+1)*4)(R13), R1

	MOVM.IA	(R13), [R0-R8]		/* restore a few user registers */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$(4*2), R13		/* space for argument+link (for debugger) */
	MOVW	$0xdeaddead, R11	/* marker */

	BL	trap(SB)		/* trap(ureg) */
	/*
	 * caller saves on plan 9, so registers other than 9, 10, 13 & 14
	 * may have been trashed when we get here.
	 */

	MOVW	$setR12(SB), R12	/* reload kernel's SB */

	ADD	$(4*2+4*NREGS), R13	/* make r13 point to ureg->type */

	/*
	 * if we interrupted a previous trap's handler and are now
	 * returning to it, we need to propagate the current R(MACH) (R10)
	 * by overriding the saved one on the stack, since we may have
	 * been rescheduled and be on a different processor now than
	 * at entry.
	 */
	MOVW	R(MACH), (-(NREGS-MACH)*4)(R13) /* restore current cpu's MACH */

	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */

	/* return from kernel-mode exception */
	MOVW	R0, SPSR		/* ... */

	/*
	 * order on stack is type, psr, pc, but RFEV7 needs pc, psr.
	 * step on type and previous word to hold temporary values.
	 * we could instead change the order in which psr & pc are pushed.
	 */
	MOVW	4(R13), R1		/* psr */
	MOVW	8(R13), R2		/* pc */
	MOVW	R2, 4(R13)		/* pc */
	MOVW	R1, 8(R13)		/* psr */

	/* restore kernel regs other than SP; we're using it */
	SUB	$(NREGS*4), R13
	MOVM.IA.W (R13), [R0-R12]
	ADD	$4, R13			/* skip saved kernel SP */
	MOVM.IA.W (R13), [R14]
	ADD	$4, R13			/* pop type, sp -> pc */
	BARRIERS
	RFEV7W(13)

	/*
	 * here for trap from USER mode
	 */
_userexcep:
	MOVM.DB.W [R0-R2], (R13)	/* set ureg->{type, psr, pc}; r13 points to ureg->type  */
	MOVM.IA	  (R3), [R0-R4]		/* restore [R0-R4] from previous mode's stack */

	/* avoid the ambiguity described in notes/movm.w. */
	MOVM.DB.S [R0-R14], (R13)	/* save kernel level registers */
	SUB	$(NREGS*4), R13		/* r13 now points to ureg */

	MOVW	$setR12(SB), R12	/* Make sure we've got the kernel's SB loaded */

	/*
	 * set up m and up registers since user registers could contain anything
	 */
	CPUID(R1)
	SLL	$2, R1			/* convert to word index */
	MOVW	$machaddr(SB), R2
	ADD	R1, R2
	MOVW	(R2), R(MACH)		/* m = machaddr[cpuid] */
	CMP	$0, R(MACH)
	MOVW.EQ	$MACHADDR, R0		/* paranoia: use MACHADDR if 0 */
	MOVW	8(R(MACH)), R(USER)	/* up = m->proc */

	MOVW	((NREGS+1)*4)(R13), R2	/* saved SPSR */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$(4*2), R13		/* space for argument+link (for debugger) */

	BL	trap(SB)		/* trap(ureg) */
	/*
	 * caller saves on plan 9, so registers other than 9, 10, 13 & 14
	 * may have been trashed when we get here.
	 */

	ADD	$(4*2+4*NREGS), R13	/* make r13 point to ureg->type */

	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */

	MOVW	4(R13), R0		/* restore SPSR */
	B	_rfue


TEXT _vfiq(SB), 1, $-4			/* FIQ */
	PUTC('?')
	PUTC('f')
	PUTC('i')
	PUTC('q')
	RFE				/* FIQ is special, ignore it for now */

TEXT _vhype(SB), 1, $-4
	PUTC('?')
	PUTC('h')
	PUTC('y')
	PUTC('p')
	RFE

/*
 *  set the stack value for the mode passed in R0
 */
TEXT setr13(SB), 1, $-4
	MOVW	4(FP), R1

	MOVW	CPSR, R2
	BIC	$(PsrMask|PsrMbz), R2, R3
	ORR	$(PsrDirq|PsrDfiq), R3
	ORR	R0, R3

	MOVW	R3, CPSR		/* switch to new mode */

	MOVW	R13, R0			/* return old sp */
	MOVW	R1, R13			/* install new one */

	MOVW	R2, CPSR		/* switch back to old mode */
	RET
