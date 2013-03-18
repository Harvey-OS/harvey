#include "mem.h"
#include "amd64l.h"

MODE $64

/*
 */
TEXT touser(SB), 1, $-4
	CLI
	SWAPGS
	MOVQ	$SSEL(SiUDS, SsRPL3), AX
	MOVW	AX, DS
	MOVW	AX, ES
	/*
	 * FS is used for TLS. Don't touch!
	MOVW	AX, FS
 	 */
	MOVW	AX, GS

	MOVQ	$(UTZERO+0x28), CX		/* ip */
	MOVQ	$If, R11			/* flags */

	MOVQ	RARG, SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */
/*
 */
TEXT linuxsyscallentry(SB), 1, $-4
_linuxsyscallentry:
	SWAPGS
	/* Wow, this is messy. LInux is callee-save. But we can't save these
	 * to kernel stack until we use them. So we push them onto user stack,
	 * and in linuxsyscall we pop them into uregs. On return, down below,
	 * we restore them from uregs. We used to just push them here and pop
	 * them below, but that made it impossible to manage notes, as the stack frame
	 * was not compatible at that point between normal NxM syscalls and Linux
	 * syscalls.
	 * see, e.g., 01814caee60a687c700d4f7303ddad8138a9c7e8 to see how it used to
	 * be done if you are curious.
	 */
	PUSHQ	RMACH
	PUSHQ	RUSER
	PUSHQ	R13
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* m->proc */
	MOVQ	SP, R13
	MOVQ	16(RUSER), SP			/* m->proc->kstack */
	ADDQ	$KSTACK, SP
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* old stack segment */
	PUSHQ	R13				/* old sp */
	PUSHQ	R11				/* old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	CX				/* old ip */

	SUBQ	$(18*8), SP			/* unsaved registers */
	MOVQ	AX, (0)(SP)
	MOVQ	BX, (1*8)(SP)
	MOVQ	CX, (2*8)(SP)
	MOVQ	DX, (3*8)(SP)
	MOVQ	SI, (4*8)(SP)
	MOVQ	DI, (5*8)(SP)
	MOVQ	BP, (6*8)(SP)
	MOVQ	R8, (7*8)(SP)
	MOVQ	R9, (8*8)(SP)
	MOVQ	R10, (9*8)(SP)
	MOVQ	R11, (10*8)(SP)
	MOVQ	R12, (11*8)(SP)

	MOVW	$SSEL(SiUDS, SsRPL3), (15*8+0)(SP)
	MOVW	ES, (15*8+2)(SP)
	MOVW	FS, (15*8+4)(SP)
	MOVW	GS, (15*8+6)(SP)

	PUSHQ	SP				/* Ureg* */
	PUSHQ	RARG				/* system call number */
	CALL	linuxsyscall(SB)

TEXT linuxsyscallreturn(SB), 1, $-4
#ifdef NOTNOW
	/* we do the TLS setup in linuxsyscall.c.
	 * Leave this here in case we ever think it should be done
	 * here.
	 */
	/* this is the time to set up TLS */
	/* TLS is held in proc->tls, offset 32 */
	/* we need to pull it into DX:AX, set C0000100 into CX,
	 * and do a WRMSR
	 */
	/* consider pushing RUSER (above) and popping it here. */
	XORL	AX, AX
	MOVW	AX, FS
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* m->proc */
	MOVQ	32(RUSER), AX			/* m->proc->tls */
	MOVQ	AX, DX
	SHRQ	$32, DX
	MOVL	$0xC0000100, CX
	WRMSR
#endif
	MOVQ	16(SP), AX			/* Ureg.ax */
	MOVQ	(16+6*8)(SP), BP		/* Ureg.bp */
_linuxsyscallreturn:
	/* aka RMACH and RUSER */
	MOVQ	(16+14*8)(SP),R15
	MOVQ	(16+13*8)(SP),R14
	MOVQ	(16+12*8)(SP),R13
	MOVQ	(16+11*8)(SP),R12
	MOVQ	(16+9*8)(SP),R10
	MOVQ	(16+8*8)(SP),R9
	MOVQ	(16+7*8)(SP),R8
	MOVQ	(16+6*8)(SP),BP
	MOVQ	(16+5*8)(SP),DI
	MOVQ	(16+4*8)(SP),SI
	MOVQ	(16+3*8)(SP),DX
	MOVQ	(16+1*8)(SP),BX
	ADDQ	$(17*8), SP			/* registers + arguments */

	/* the CLI should maybe be done sooner */
	CLI
	SWAPGS
	MOVW	0(SP), DS
	MOVW	2(SP), ES
	/* This breaks TLS.
	MOVW	4(SP), FS
	*/
	MOVW	6(SP), GS

	MOVQ	24(SP), CX			/* ip */
	MOVQ	40(SP), R11			/* flags */

	MOVQ	48(SP), SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

TEXT linuxsysrforkret(SB), 1, $-4
	CALL	linuxclonefinish(SB)
	MOVQ	$0, AX
	JMP	_linuxsyscallreturn

/*
 */
TEXT syscallentry(SB), 1, $-4
	TESTQ	$0x8000, AX
	JZ	_linuxsyscallentry
	SWAPGS
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* m->proc */
	MOVQ	SP, R13
	MOVQ	16(RUSER), SP			/* m->proc->kstack */
	ADDQ	$KSTACK, SP
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* old stack segment */
	PUSHQ	R13				/* old sp */
	PUSHQ	R11				/* old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	CX				/* old ip */

	SUBQ	$(18*8), SP			/* unsaved registers */
	/* register arguments */
	/* saved in the order in which they are used. */
	MOVQ	DI, (5*8)(SP)
	MOVQ	SI, (4*8)(SP)
	MOVQ	DX, (3*8)(SP)
	MOVQ	R10, (9*8)(SP)
	MOVQ	R8, (7*8)(SP)

	MOVW	$SSEL(SiUDS, SsRPL3), (15*8+0)(SP)
	MOVW	ES, (15*8+2)(SP)
	MOVW	FS, (15*8+4)(SP)
	MOVW	GS, (15*8+6)(SP)

	PUSHQ	SP				/* Ureg* */
	PUSHQ	RARG				/* system call number */
	CALL	syscall(SB)

TEXT syscallreturn(SB), 1, $-4
	MOVQ	16(SP), AX			/* Ureg.ax */
	MOVQ	(16+6*8)(SP), BP		/* Ureg.bp */
_syscallreturn:
	ADDQ	$(17*8), SP			/* registers + arguments */

	CLI
	SWAPGS
	MOVW	0(SP), DS
	MOVW	2(SP), ES
	/* Linux system calls can be interleaved with NIX
	 * system calls. This would break TLS if we left it in.
	MOVW	4(SP), FS
	 */
	MOVW	6(SP), GS

	MOVQ	24(SP), CX			/* ip */
	MOVQ	40(SP), R11			/* flags */

	MOVQ	48(SP), SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

TEXT sysrforkrettrace(SB), 1, $-4
	CALL rforktraceend(SB)
TEXT sysrforkret(SB), 1, $-4
	MOVQ	$0, AX
	JMP	_syscallreturn
