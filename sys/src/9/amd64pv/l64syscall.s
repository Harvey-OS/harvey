#include "mem.h"
#include "amd64l.h"

.code64

/*
 */
.globl touser
touser:
	CLI
	SWAPGS
	MOVQ	$SSEL(SiUDS, SsRPL3), %rax
	MOVW	%ax, %DS
	MOVW	%ax, %ES
	MOVW	%ax, %FS
	MOVW	%ax, %GS

	MOVQ	$(UTZERO+0x28), %rcx		/* ip */
	MOVQ	$If, %r11			/* flags */

	MOVQ	%rdi, %rsp			/* sp */

	.byte 0x48; SYSRET			/* SYSRETQ */

/*
 */
.globl syscallentry
syscallentry:
	SWAPGS
	.byte 0x65; MOVQ 0, %r15		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(%r15),%r14     		/* m->proc */
	MOVQ	%rsp, %r13
	MOVQ	16(RUSER), %rsp			/* m->proc->kstack */
	ADDQ	$KSTACK, %rsp
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* old stack segment */
	PUSHQ	%r13				/* old sp */
	PUSHQ	%r11				/* old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	%rcx				/* old ip */

	SUBQ	$(18*8), %rsp			/* unsaved registers */

	MOVW	$SSEL(SiUDS, SsRPL3), (15*8+0)(%rsp)
	MOVW	%ES, (15*8+2)(%rsp)
	MOVW	%FS, (15*8+4)(%rsp)
	MOVW	%GS, (15*8+6)(%rsp)

	PUSHQ	%rsp				/* Ureg* */
	PUSHQ	%rdi				/* system call number */
	CALL	syscall

.globl syscallreturn
syscallreturn:
	MOVQ	16(%rsp), AX			/* Ureg.ax */
	MOVQ	(16+6*8)(%rsp), BP		/* Ureg.bp */
_syscallreturn:
	ADDQ	$(17*8), %rsp			/* registers + arguments */

	CLI
	SWAPGS
	MOVW	0(%rsp), %DS
	MOVW	2(%rsp), %ES
	MOVW	4(%rsp), %FS
	MOVW	6(%rsp), %GS

	MOVQ	24(%rsp), %rcx			/* ip */
	MOVQ	40(%rsp), %r11			/* flags */

	MOVQ	48(%rsp), %rsp			/* sp */

	.byte 0x48; SYSRET			/* SYSRETQ */

.globl sysrforkret
sysrforkret:
	// DEBUG
	cli
1: jmp 1b
	MOVQ	$0, %rax
	JMP	_syscallreturn
