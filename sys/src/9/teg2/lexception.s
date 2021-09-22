/*
 * arm exception handlers
 *
 * Aside from reset, the exception handlers primarily save the appropriate
 * registers (depending upon the previous processor mode), re-establish R(MACH)
 * and R(USER) if needed, call syscall() or trap() in C, cope with possible
 * rescheduling onto another cpu, restore the saved registers and return
 * from exception.  The system call entry, _vsvc, is a prototype for the
 * trap handlers and is more heavily commented.
 */
#include "arm.s"

/*
 *  exception vectors, copied by l.s and vecinit to somewhere useful.
 *  assumes vectors, vtable, wfiloop, and _vrst are contiguous.
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

	/* addresses to serve corresponding vectors above */
	WORD	$_vrst-KZERO(SB)	/* reset, in svc mode already */
	WORD	$_vund(SB)		/* undefined, switch to svc mode */
	WORD	$_vsvc(SB)		/* swi, in svc mode already */
	WORD	$_vpabt(SB)		/* prefetch abort, switch to svc mode */
	WORD	$_vdabt(SB)		/* data abort, switch to svc mode */
	WORD	$_vhype(SB)		/* hypervisor call */
	WORD	$_virq(SB)		/* IRQ, switch to svc mode */
	WORD	$_vfiq(SB)		/* FIQ, switch to svc mode */

/*
 * idle forever.  this should be safe, once copied to low memory,
 * across reboots.  update trapinit() if you change this routine.
 */
TEXT _wfiloop(SB), 1, $-4
wfiagain:
	WFI
	B	wfiagain

/*
 * reset - start secondary cpus.
 *
 * invoked on cpu(s) other than 0, to start them, with interrupts off
 * and in SVC mode, running in the zero segment (pc is in lower 256MB).
 * L2 cache and scu are on, and our L1 D cache and mmu should be off.
 * This cpu's Mach was initialised by launchinit.
 */
TEXT _vrst(SB), 1, $-4
	MOVW	$(PsrMsvc|PsrDirq|PsrDfiq), CPSR  /* svc mode, interrupts off */
	BARRIERS
	CLREX
	SETZSB

	MOVW	$PsrMsvc, SPSR
	MOVW	$0, R14			/* no return possible */

	/* invalidate i-cache and branch-target cache */
	MTCP	CpSC, 0, PC, C(CpCACHE), C(CpCACHEinviis), CpCACHEall
	BARRIERS

	/* cpu non-0 step 1: mpcore manual: turn my mmu and L1 D cache off. */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpCsbo, R0
	BIC	$(CpCsbz|CpCmmu|CpCdcache|CpCpredict), R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/* reuse cpu0's temporary stack and get this cpu into a known state. */
	MOVW	$((PHYSDRAM | TMPSTACK) - ARGLINK), SP
	BL	setmach(SB)		/* m will be in zero seg */
	/* can now call arbitrary C functions */

	CPUID(R0)
	BL	seccpustart(SB)		/* to C, never to return */
	B	wfiagain		/* "can't happen" */

/*
 * system call
 */
TEXT _vsvc(SB), 1, $-4			/* SWI */
	MOVW	$(PsrMsvc|PsrDirq|PsrDfiq), CPSR  /* svc mode, interrupts off */
	BARRIERS
	CLREX

	/*
	 * we must push registers onto the stack in the exact reverse order
	 * of ureg.h.
	 */
	/* stack is m->stack */
	MOVW.W	R14, -4(R13)		/* ureg->pc = interrupted PC */
	MOVW	SPSR, R14		/* ureg->psr = SPSR */
	MOVW.W	R14, -4(R13)		/* ... */
	MOVW	$PsrMsvc, R14		/* ureg->type = PsrMsvc */
	MOVW.W	R14, -4(R13)		/* ... */

	/*
	 * avoid the ambiguity described in notes/movm.w.
	 * In order to get a predictable value in R13 after the stores,
	 * separate the store-multiple from the stack-pointer adjustment.
	 * We'll assume that the old value of R13 should be stored on the stack.
	 */
	MOVM.DB.S [R0-R14], (R13)	/* save user level registers */
	SUB	$(NREGS*BY2WD), R13	/* r13 now points to ureg */

	/*
	 * set up m and up registers since user registers could contain anything
	 */
	MOVW	$setR12(SB), R12	/* load the kernel's SB */
	/* no need to save LR */
	BL	setmach(SB)		/* clobbers R1-R4 */
	MOVW	8(R(MACH)), R(USER)	/* up = m->proc */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$ARGLINK, R13		/* space for argument+link */
/**/	BL	syscall(SB)

	/*
	 * caller saves on plan 9, so registers other than 9, 10, 12, 13 & 14
	 * (up, m, sb, sp, lr) may have been trashed when we get here.
	 */
	MOVW	$setR12(SB), R12	/* reload kernel's SB */
_rfutrap:
	/*
	 * after this addition, there must be no interrupts until after RFE
	 * because we continue to refer to saved registers below the stack
	 * pointer.  we must also not push anything onto the stack in the
	 * interim.
	 */
	ADD	$(ARGLINK+BY2WD*NREGS), R13 /* make r13 point to ureg->type */
	MOVW	BY2WD(R13), R0		/* restore SPSR */
	/* fall through */
/*
 * return from user-mode exception.
 * expects new SPSR in R0.  R13 must point to ureg->type.
 */
_rfue:
TEXT rfue(SB), 1, $-4
	MOVW	R0, SPSR		/* ... */
	BL	swappcpsr(SB)		/* clobbers R1, R2 only */

	/* note that this restores m and up */
	MOVM.DB.S (R13), [R0-R14]	/* restore user registers */
	ADD	$BY2WD, R13		/* pop type; sp -> pc */
	RFEV7W(13)			/* back to user */


/*
 * converts address in R0 to the current segment, as defined by the PC.
 * clobbers R1 & R2.
 */
TEXT addr2pcseg(SB), 1, $-4
	MOVW	$KSEGM, R2
	BIC	R2, R0
	MOVW	PC, R1
	AND	R2, R1			/* segment PC is in */
	ORR	R1, R0
	RET

/*
 * sets R(MACH) from machaddr[cpuid], returns it in R0.  clobbers R1-R4.
 * doesn't use the stack.  can be called from zero or KZERO segment.
 */
TEXT setmach(SB), 1, $-4
	MOVW	R14, R4

	CPUID(R3)
	SLL	$2, R3			/* convert to word index */

	MOVW	$machaddr(SB), R0
	BL	addr2pcseg(SB)
	ADD	R3, R0			/* R0 = &machaddr[cpuid] */
	MOVW	(R0), R0		/* R0 = machaddr[cpuid] */

	CMP	$0, R0
	MOVW.EQ	$MACHADDR, R0		/* paranoia: use MACHADDR if 0 */

	BL	addr2pcseg(SB)
	MOVW	R0, R(MACH)		/* m = machaddr[cpuid] */

	MOVW	R4, R14
	RET

/*
 * order on stack is {type, psr, pc}, but RFEV7 needs {pc, psr}.
 * we could instead change the order in which psr & pc are pushed,
 * at the cost of compatibility with older arm kernels, debuggers, etc.
 * since ureg.h would have to change to match the new order.
 */
TEXT swappcpsr(SB), 1, $-4
	MOVW	BY2WD(R13), R1		/* psr */
	MOVW	(2*BY2WD)(R13), R2	/* pc */
	MOVW	R2, BY2WD(R13)		/* pc */
	MOVW	R1, (2*BY2WD)(R13)	/* psr */
	RET

TEXT _vund(SB), 1, $-4			/* undefined */
	CPSID
	/* sp is m->sund */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMund, R0
	B	_vswitch

TEXT _vpabt(SB), 1, $-4			/* prefetch abort */
	CPSID
	/* sp is m->sabt */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMabt, R0		/* r0 = type */
	B	_vswitch

TEXT _vdabt(SB), 1, $-4			/* data abort */
	CPSID
	/* sp is m->sabt */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$(PsrMabt+1), R0	/* r0 = type */
	B	_vswitch

TEXT _virq(SB), 1, $-4			/* IRQ */
	CPSID
	/* sp is m->sirq */
	MOVM.IA	[R0-R4], (R13)		/* free some working space */
	MOVW	$PsrMirq, R0		/* r0 = type */
	/* fall through */

	/*
	 *  come here with type in R0 and R13 pointing above saved [r0-r4].
	 *  we'll switch to SVC mode and then call trap.
	 */
_vswitch:
	BARRIERS
	CLREX

	MOVW	SPSR, R1		/* save SPSR for ureg */
	/*
	 * R12 needs to be set before using PsrMbz.
	 */
	MOVW	R14, R2			/* save interrupted pc for ureg */
	MOVW	R13, R3	/* save pointer to where the original [R0-R4] are */

	/*
	 * switch processor to svc mode.  this switches the banked registers
	 * (r13 [sp] and r14 [link]) to those of svc mode (so we must be sure
	 * to never get here already in svc mode).  note that this complicates
	 * the possibility of nested interrupts.
	 */
	MOVW	$(PsrMsvc|PsrDirq|PsrDfiq), CPSR  /* svc mode, interrupts off */
	BARRIERS
	/* R13 is now the Msvc banked one, somewhere in m->stack */

	AND.S	$0xf, R1, R4	/* interrupted code was kernel or user? */
	BEQ	_userexcep

/*
 * here for trap from SVC (kernel) mode
 */

	/*
	 * push ureg->{type, psr, pc} onto Msvc stack.
	 * r13 points to ureg->type after.
	 */
	MOVM.DB.W [R0-R2], (R13)
	MOVM.IA	  (R3), [R0-R4]	/* restore [R0-R4] from previous mode's stack */

	/* save kernel level registers, at end r13 points to ureg */
	MOVM.DB	[R0-R14], (R13)
	SUB	$(NREGS*BY2WD), R13	/* SP now points to saved R0 */

	MOVW	$setR12(SB), R12	/* load the kernel's SB */
	/* we assume that m & up are still correctly set */
	/* previous mode was svc, so the saved spsr should be sane. */

	MOVM.IA	(R13), [R0-R8]		/* restore user scratch registers */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$ARGLINK, R13		/* space for arg+link (for debugger) */
	MOVW	$0xdeaddead, R11	/* marker */
/**/	BL	trap(SB)		/* trap(ureg) */

	/*
	 * caller saves on plan 9, so registers other than 9, 10, 13 & 14
	 * (up, m, sp, lr) may have been trashed when we get here.
	 */
	MOVW	$setR12(SB), R12	/* reload kernel's SB */
	/* no interrupts until after RFE. */
	ADD	$(ARGLINK+BY2WD*NREGS), R13 /* make r13 point to ureg->type */

	/*
	 * if we interrupted a previous trap's handler and are now
	 * returning to it, we need to propagate the current R(MACH) (R10)
	 * by overriding the saved one on the stack, since we may have
	 * been rescheduled and be on a different processor now than
	 * at entry.
	 */
	/* no need to save LR */
	BL	setmach(SB)		/* clobbers R1-R4 */
	/* restore current cpu's MACH */
	MOVW	R(MACH), (-(NREGS-MACH)*BY2WD)(R13)
	/* up should not have changed; we are the same process */

	/* return from kernel-mode exception */
	MOVW	BY2WD(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */

	/* no need to save LR */
	BL	swappcpsr(SB)		/* clobbers R1, R2 only */

	/* restore kernel regs other than SP; we're using it */
	SUB	$(NREGS*BY2WD), R13
	MOVM.IA.W (R13), [R0-R12]	/* note that this restores m and up */
	ADD	$BY2WD, R13		/* skip saved kernel SP */
	MOVM.IA.W (R13), [R14]

	ADD	$BY2WD, R13		/* pop type; sp -> pc */
	BARRIERS
	RFEV7W(13)			/* back to kernel */

/*
 * here for trap from USER mode
 */
_userexcep:
	/* push ureg->{type, psr, pc}; r13 points to ureg->type */
	MOVM.DB.W [R0-R2], (R13)
	MOVM.IA	  (R3), [R0-R4]	/* restore [R0-R4] from previous mode's stack */

	/* avoid the ambiguity described in notes/movm.w. */
	MOVM.DB.S [R0-R14], (R13)	/* save kernel level registers */
	SUB	$(NREGS*BY2WD), R13	/* r13 now points to ureg */

	MOVW	$setR12(SB), R12	/* load the kernel's SB */
	/*
	 * set up m and up registers since user registers could contain anything
	 */
	/* no need to save LR */
	BL	setmach(SB)		/* clobbers R1-R4 */
	MOVW	8(R(MACH)), R(USER)	/* up = m->proc */

	MOVW	R13, R0			/* first arg is pointer to ureg */
	SUB	$ARGLINK, R13		/* space for arg+link (for debugger) */
/**/	BL	trap(SB)		/* trap(ureg) */
	/*
	 * we may have been rescheduled onto a different cpu, but
	 * saved user register values will be restored soon by rfue.
	 */
	B	_rfutrap		/* back to user */


TEXT _vfiq(SB), 1, $-4			/* FIQ */
	CPSID
	PUTC('?'); PUTC('f'); PUTC('i'); PUTC('q')
	RFE				/* FIQ is special, ignore it for now */

TEXT _vhype(SB), 1, $-4
	CPSID
	PUTC('?'); PUTC('h'); PUTC('y'); PUTC('p')
	RFE

/*
 *  set the stack pointer value at 4(FP) for the mode passed in R0.
 */
TEXT setr13(SB), 1, $-4
	MOVW	4(FP), R1		/* new sp */

	MOVW	CPSR, R2
	BIC	$(PsrMask|PsrMbz), R2, R3
	ORR	$(PsrDirq|PsrDfiq), R3
	ORR	R0, R3

	MOVW	R3, CPSR		/* switch to new mode */
	BARRIERS

	MOVW	R13, R0			/* return old sp */
	MOVW	R1, R13			/* install new one */

	MOVW	R2, CPSR		/* switch back to old mode */
	BARRIERS
	RET
