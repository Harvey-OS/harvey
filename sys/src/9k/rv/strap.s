/*
 * Trap catcher for supervisor mode of RV64.
 *
 * The strategy is to exchange R(MACH) and CSR(SSCRATCH), which is pre-loaded
 * with this CPU's Mach pointer, save all registers and a few CSRs on a stack
 * (empty up->kstack if trap came from user mode), call the common trap()
 * function, restore most or all of the stacked registers and CSRs, exchange
 * R(MACH) and CSR(SSCRATCH), and return from trap via SRET to the address in
 * CSR(SEPC).  Depending upon trap()'s action, the interrupted program could
 * potentially continue unaware of the trap.
 *
 * Note well that the exact stack layout of a trap is known in various other
 * places too (e.g., main.c, mmu.c, trap.c).  The intended layout is: a Ureg
 * struct at the very end of up->kstack if trapped from user mode, or
 * somewhere on m->stack if trapped from kernel, with a pointer to that Ureg
 * just before (below) it, for the argument to trap().
 */

#include "mem.h"
#include "riscv64l.h"

#define SPLHI	CSRRC	CSR(SSTATUS), $Sie, R0
#define SPLLO	CSRRS	CSR(SSTATUS), $Sie, R0

#define CALLTRAP \
	MOV	R2, R(ARG);		/* Ureg* */\
	MOV	$(Sum|Mxr), R9;\
	CSRRS	CSR(SSTATUS), R9, R0;\
	SUB	$(2*XLEN), R2; /* leave needed room (return PC & dummy arg?) */\
/**/	JAL	LINK, trap(SB);		/* to C: trap(sp) */\
	ADD	$(2*XLEN), R2

/* side-effect: patches Ureg with regsave's R2 */
#define PUSHALLS \
	SUB	$UREGSIZE, R2;		/* room for pc, regs, csrs */\
	SAVER1_31;\
	/* patch up Ureg on stack */\
	MOV	SAVESR2(R(MACH)), R9;\
	MOV	R9, (2*XLEN)(R2);	/* push saved R2 from regsave */\
	/* save CSRs */\
	MOV	CSR(SEPC), R9;		MOV R9, 0(R2);	/* push saved pc */\
	MOV	CSR(SSTATUS), R9;	MOV R9, (32*XLEN)(R2);\
	MOV	CSR(SIE), R9;		MOV R9, (33*XLEN)(R2);\
	MOV	CSR(SCAUSE), R9;	MOV R9, (34*XLEN)(R2);\
	MOV	CSR(STVAL), R9;		MOV R9, (35*XLEN)(R2);\
	MOV	$Mppsuper, R9;		MOV R9, (36*XLEN)(R2);\
	MOV	CSR(SSCRATCH), R9;	MOV R9, (37*XLEN)(R2)

#define POPMOSTREGS \
	POPR8_31; \
	MOV	(1*XLEN)(R2), R1;	/* restore link reg */ \
	MOV	(4*XLEN)(R2), R4;	/* restore saved temporary */ \
	MOV	(5*XLEN)(R2), R5

/*
 * mips ports restore only M(STATUS) and M(EPC), but have no *IE.
 * there should be no need (or good reason) to restore SIE
 * (and SSCRATCH, STVAL, and SCAUSE).
 */
#define POPCSRS \
	MOV         0(R2), R4; MOV R4, CSR(SEPC); \
	MOV	$dummysc(SB), R9; \
	SCW(0, 9, 0); \
	MOV (32*XLEN)(R2), R4; MOV R4, CSR(SSTATUS)	/**/

/*
 * strap copes with supervisor-mode faults, from user or kernel mode
 * (e.g., page fault during system call).
 * the trap mechanism saved the return address in CSR(SEPC), and masked
 * interrupts.  Upon entry, it may be necessary to save a few registers
 * in m->regsave[] to free them for use until all the registers are saved.
 * SP should be valid on entry from kernel mode, at least.
 *
 * upon return from trap(), be sure to pop user's registers from Ureg, so
 * that changes to this Ureg elsewhere (e.g. trap, sysexec) are reflected
 * back to the user process.
 */

TMP=9
UART0=10
TMP2=11

TEXT recktrap(SB), 1, $-4
TEXT recutrap(SB), 1, $-4
	/* fault in prologue: fix registers most likely to be damaged */
	MOV	$setSB(SB), R3
	MOV	$panicstk+(3*PGSZ)(SB), R2
	AND	$~(8ll-1), R2			/* align */
TEXT rectrapalign(SB), 1, $-4
	MOV	$PAUart0, R(UART0)		/* Thr */
	MOVW	$(1ul<<31), R(TMP2)		/* sifive tx notready bit */
busy:
	FENCE
#ifdef SIFIVEUART
	MOVWU	(0*4)(R(UART0)), R(TMP)		/* reg 0 is txdata */
	AND	R(TMP2), R(TMP)			/* notready bit */
	BNE	R(TMP), busy
#else
	MOVWU	(5*4)(R(UART0)), R(TMP)
	AND	$0x20, R(TMP)			/* Thre */
	BEQ	R(TMP), busy
#endif
	MOV	$'\r', R(TMP)
	MOVW	R(TMP), (R(UART0))
	MOV	$'\n', R(TMP)
	MOVW	R(TMP), (R(UART0))
	MOV	$'?', R(TMP)
	MOVW	R(TMP), (R(UART0))
	MOVW	R(TMP), (R(UART0))
	MOV	$'\r', R(TMP)
	MOVW	R(TMP), (R(UART0))
	MOV	$'\n', R(TMP)
	MOVW	R(TMP), (R(UART0))

	/* this may just produce a panic, but it's better than a silent loop */
//	BEQ	R0, cope

	MOV	R0, R8
	SUB	$(2*XLEN), R2
	MOV	R0, (R2)
	JAL	LINK, badstack(SB)
wfi:
	WFI
	JMP	wfi

TEXT strap(SB), 1, $-4
	/* this should be a no-op if we trapped from kernel mode. */
	CSRRW	CSR(SSCRATCH), R(MACH), R(MACH)	/* save old m, load new m */
	FENCE

	/* stash a few regs in m->regsave to free working registers */
	MOV	R2, SAVESR2(R(MACH))		/* save SP */
	MOV	R3, SAVESR3(R(MACH))		/* save SB */
	MOV	R4, SAVESR4(R(MACH))		/* save temp */
	MOV	R9, SAVESR9(R(MACH))

cope:
	MOV	CSR(SSTATUS), R9
	AND	$Spp, R9
	BEQ	R9, fromuser			/* prev not super */

	/*
	 * trapped from kernel mode: R2, SB, MACH, and USER were all okay,
	 * in theory.  this can't be a system call.
	 * we shall use the current kernel stack.
	 */
	FENCE
	MOV	$recktrap(SB), R9
	MOV	R9, CSR(STVEC)		/* if we fault here, cope */

	/* if we fault here, it's probably due to a bad SP or SB */
	MOV	SAVESR4(R(MACH)), R4
	MOV	SAVESR9(R(MACH)), R9
	PUSHALLS			/* patches R2 from regsave into Ureg */

	FENCE
	MOV	$strap(SB), R9
	MOV	R9, CSR(STVEC)		/* restore trap vector */

	MOV	$setSB(SB), R3	/* should be redundant, barring disaster */
/**/	CALLTRAP  /* C compiler is free to write regs but R2, SB, USER, MACH */
	SPLHI
	POPCSRS
	POPMOSTREGS

	/* MACH, SB are still correct; USER might not be after resched? */
	MOV	MACHPROC(R(MACH)), R(USER)	/* up = m->proc */
	JMP	return			/* to kernel */

	/*
	 * trapped from user mode: SB, MACH, R2, USER all need(ed) reloading.
	 * old MACH value is in CSR(SSCRATCH).  R2 is saved in regsave.
	 */
fromuser:
	MOV	$setSB(SB), R3

	FENCE
	MOV	$recutrap(SB), R9
	MOV	R9, CSR(STVEC)		/* if we fault here, cope */

	MOV	SAVESR9(R(MACH)), R9

	MOV	R4, SAVESR4(R(MACH))	/* assembler temp */
	MOV	R6, SAVESR6(R(MACH))	/* save old R6 (now up) */
	MOV	MACHPROC(R(MACH)), R(USER) /*  up = m->proc; up is R6 */

	/*
	 * can't use user-mode stack.  switch to up->kstack.
	 */
	MOV	(2*XLEN)(R(USER)), R2	/* switch to empty up->kstack */
	ADD	$KSTACK, R2		/* must be multiple of sizeof(vlong) */

	PUSHALLS			/* patches R2 from regsave into Ureg */
	MOV	SAVESR3(R(MACH)), R9
	MOV	R9, (3*XLEN)(R2)	/* patch old R3 into Ureg */
	MOV	SAVESR4(R(MACH)), R4
	MOV	R4, (4*XLEN)(R2)	/* patch old R4 into Ureg */
	MOV	SAVESR6(R(MACH)), R9
	MOV	R9, (6*XLEN)(R2)	/* patch old R6 into Ureg */

	FENCE
	MOV	$strap(SB), R9
	MOV	R9, CSR(STVEC)		/* restore trap vector */

	/* sched could be called and processes switched (e.g, syscall) */
/**/	CALLTRAP  /* C compiler is free to write regs but R2, SB, MACH, USER */
	SPLHI

syscallret:
	/* SP must point at our Ureg here.  up may have changed since entry. */
	/*
	 * before a syscall, the compiler pushed any registers it cared about
	 * (other than those it considers global, like SB, R2, MACH, USER),
	 * so we needn't save nor restore most of them before returning
	 * from a syscall.  We do need to propagate ureg->pc and ureg->sp
	 * back into user registers.
	 */
	POPCSRS
	MOV	$(Sum|Mxr), R6
	CSRRS	CSR(SSTATUS), R6, R0
	POPMOSTREGS
	MOV	(3*XLEN)(R2), R3	/* restore user's SB */
	MOV	(6*XLEN)(R2), R6
return:
	MOV	(2*XLEN)(R2), R2	/* restore SP */
	FENCE
	CSRRW	CSR(SSCRATCH), R(MACH), R(MACH) /* restore R7, reload saved m */
	SRET				/* to CSR(SEPC) address */

/* rfork return for child processes */
TEXT sysrforkret(SB), 1, $-4
	SPLHI

	/* SP must point at our Ureg here.  up may have changed since entry. */
	MOV	R0, (ARG*XLEN)(R2)	/* zero saved R(ARG) in ureg */
	JMP	syscallret

/*
 * switch to user mode with stack pointer from sp+0(FP) at start of text.
 * used to start process 1 (init).
 */
TEXT touser(SB), 1, $-4
	SPLHI		/* ensure intrs see consistent kernel or user regs */

	FENCE
	FENCE_I

	MOV	CSR(SSTATUS), R13
	MOV	$~(Ube|Spp|Uxl), R12	/* little endian, prev mode = user */
	AND	R12, R13
	MOV	$(Spie|Mxlen64<<Uxlshft|Sum|Mxr), R12
	OR	R12, R13	/* new flags: allow user interrupts */
	MOV	R13, CSR(SSTATUS)

	MOV	R0, LINK
	MOV	$(UTZERO+0x20), R12
	MOV	R12, CSR(SEPC)		/* new pc */
	MOV	R(ARG), R2		/* new sp */
	SRET				/* off to rv64 user mode */

	GLOBL	dummysc(SB), $4
	GLOBL	panicstk(SB), $(3*PGSZ)
