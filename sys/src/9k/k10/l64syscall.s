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
	MOVW	AX, FS
	MOVW	AX, GS

	MOVQ	$(UTZERO+0x28), CX		/* ip */
	MOVQ	$If, R11			/* flags */

	MOVQ	RARG, SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

/*
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
	PUSHQ	R11				/* old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	CX				/* old ip */

	SUBQ	$(18*8), SP			/* unsaved registers */

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
	MOVW	4(SP), FS
	MOVW	6(SP), GS

	MOVQ	24(SP), CX			/* ip */
	MOVQ	40(SP), R11			/* flags */

	MOVQ	48(SP), SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

TEXT sysrforkret(SB), 1, $-4
	MOVQ	$0, AX
	JMP	_syscallreturn
