/*
 * assembler interface to sbi firmware for rv64.
 * call this splhi().
 *
 * SBI is a sort of BIOS and may have some of the same bugs, notably
 * unmasking interrupts and then returning.  We have put a wrapper around
 * SBI calls to save and restore interrupt enable to try to defeat this.
 * A special trap catcher should catch such nonsense.
 *
 * R10 contains my hart id at start.
 *
 * This interface follows the risc-v unix system call abi, which follows the
 * calling conventions of risc-v elf psabi: the same as function calls
 * (presumably gcc's) except that the call instruction is ECALL, and a7 (R17)
 * contains the sbi ulong extension id.  R16 may hold a ulong additional
 * function ID. SBI returns error code (`sbiret') in R10, value in R11.  The
 * error code is the success indicator, negatives are failures.
 *
 * What the risc-v abi calls a0 is R10, ...  a7 is R17.  Initial arguments go
 * in a0-a5 (a6 & a7 are reserved by sbi).  In rv32, vlongs go in even-odd
 * pairs.  sp points at first arg that won't fit in registers.  The abi
 * requires sp to be 16-byte aligned.  we have to enforce that manually; jc
 * won't.  r5-7 and r28-31 may be trashed (per the risc-v c calling standard).
 * jc will have saved r28-31 if they were in use.
 */
#include "mem.h"
#include "riscv64l.h"

/* supervisor mode */
TEXT sbiecall(SB), 0, $-4	/* sbiecall(ext, func, arg1, retp, xargs[2]) */
	MOV	$recktrap(SB), R24	/* catch super traps during sbi call */
	MOV	R24, CSR(STVEC)

	MOV	R5, R24			/* save vulnerable extern registers */
	MOV	R6, R25
	MOV	R7, R26
	MOV	retp+(3*XLEN)(FP), R23
	MOV	xargs+(4*XLEN)(FP), R22	/* extra args: r11, r12 values */

	MOV	R8, R17			/* pass ecall args: R8 is ext */
	MOV	func+XLEN(FP), R16
	MOV	arg1+(2*XLEN)(FP), R10
	MOV	R0, R11
	MOV	R0, R12
	BEQ	R22, noxargs
	MOV	(R22), R11
	MOV	XLEN(R22), R12
noxargs:

	MOV	R2, R27
	AND	$(~(SBIALIGN-1ll)), R2	/* align stack */
/**/	ECALL				/* trap to machine mode */
	MOV	R27, R2

	BEQ	R23, noret
	MOV	R11, (R23)		/* R23 is Sbiretp* */
	MOV	R10, XLEN(R23)
noret:

	MOV	R26, R7
	MOV	R25, R6
	MOV	R24, R5

	MOV	$strap(SB), R24		/* restore super trap catcher */
	MOV	R24, CSR(STVEC)

	MOV	R10, R8			/* return error code */
	RET
