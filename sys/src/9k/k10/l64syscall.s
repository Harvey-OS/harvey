#include "mem.h"
#include "amd64l.h"

MODE $64

/*
 * switch to user mode with stack pointer from sp+0(FP) as start of text.
 * used to start process 1 (init).
 */
TEXT touser(SB), 1, $-4
	CLI		/* ensure intrs see consistent kernel or user regs */
	SWAPGS
	MOVQ	$SSEL(SiUDS, SsRPL3), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS

	MOVQ	$(UTZERO+0x28), CX		/* ip */
	MOVQ	$If, R11			/* flags: allow interrtupts */

	MOVQ	RARG, SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

/*
 * entry here is from a SYSCALL instruction.  set up m & up, switch stacks,
 * save regs, call syscall(syscallno, Ureg *).  upon return, restore regs
 * including sp, and return.
 */
TEXT syscallentry(SB), 1, $-4
	SWAPGS
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* m->proc */
	MOVQ	SP, R13
	MOVQ	16(RUSER), SP			/* m->proc->kstack */
	ADDQ	$KSTACK, SP
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* old stack segment */
	PUSHQ	R13				/* old sp */
	PUSHQ	R11				/* old flags; from SYSCALL */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	CX				/* old ip; from SYSCALL */

	SUBQ	$(18*8), SP			/* unsaved registers */

	MOVW	$SSEL(SiUDS, SsRPL3), (15*8+0)(SP)
	MOVW	ES, (15*8+2)(SP)
	MOVW	FS, (15*8+4)(SP)
	MOVW	GS, (15*8+6)(SP)

	PUSHQ	SP				/* Ureg* */
	PUSHQ	RARG				/* system call number */
	CALL	syscallamd(SB)

TEXT syscallreturn(SB), 1, $-4
	MOVQ	16(SP), AX			/* Ureg.ax */
	MOVQ	(16+6*8)(SP), BP		/* Ureg.bp */
_syscallreturn:
	ADDQ	$(17*8), SP			/* registers + arguments */

	CLI		/* ensure intrs see consistent kernel or user regs */
	SWAPGS
	MOVW	0(SP), DS
	MOVW	2(SP), ES
	MOVW	4(SP), FS
	MOVW	6(SP), GS

	MOVQ	24(SP), CX			/* ip for SYSRET */
	MOVQ	40(SP), R11			/* flags for SYSRET */

	MOVQ	48(SP), SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

TEXT sysrforkret(SB), 1, $-4
	XORQ	AX, AX
	JMP	_syscallreturn
