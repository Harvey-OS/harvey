#include "mem.h"
#include "amd64.h"
#ifndef __ASSEMBLER__
#define __ASSEMBLER__
#endif

.globl sce
.globl scx
/*
 * starting the user program up. First time.
 */
	.globl touser
touser:
	CLI
	SWAPGS
	// we should be able to skip this step. We'll see.
	MOVQ	$SSEL(SiUDS, SsRPL3), %rax
	MOVW	%ax, %ds
	MOVW	%ax, %es
	/*
	 * FS is used for TLS. Don't touch!
	MOVW	AX, FS
 	 */
	MOVW	%ax, %gs
	// Hmm. Assuumes many things, eh? Assumes plan 9 a.out
	// format. This will fail.
	MOVQ	$(UTZERO+0x20), %rcx		/* ip */
	MOVQ	$If, %R11			/* flags */
	MOVQ	%RDI, %RSP			/* sp */
	sysretq

	.globl syscallentry
syscallentry:
	incq	sce
	SWAPGS

	movq	%r15, %gs:32	/* stash r15 to m->rathole */

	movq	%gs:16, %r15	/* m->proc */
	movq	(16*8)(%r15), %r15 /* m->proc->kstack */
	xchgq	%r15, %rsp
	addq	$KSTACK, %rsp

	// start building ureg
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* ureg.ss old stack segment */
	PUSHQ	%r15				/* ureg.sp old sp */
	PUSHQ	%r11				/* ureg.flags old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* ureg.cs old code segment */
	PUSHQ	%rCX				/* ureg.ip old ip */

	movq	%gs:32, %r15 	/* restore r15 from m->rathole */

	SUBQ	$(17*8), %rsp

	MOVQ	%rAX, (0*8)(%rsp)	// ureg.ax
	MOVQ	%rBX, (1*8)(%rsp)	// ureg.bx
	MOVQ	%rCX, (2*8)(%rsp)	// ureg.cx
	MOVQ	%rDX, (3*8)(%rsp)	// ureg.dx
	MOVQ	%rSI, (4*8)(%rsp)	// ...
	MOVQ	%rDI, (5*8)(%rsp)
	MOVQ	%rBP, (6*8)(%rsp)
	MOVQ	%r8, (7*8)(%rsp)
	MOVQ	%r9, (8*8)(%rsp)
	MOVQ	%r10, (9*8)(%rsp)
	MOVQ	%r11, (10*8)(%rsp)
	MOVQ	%r12, (11*8)(%rsp)
	MOVQ	%r13, (12*8)(%rsp)
	MOVQ	%r14, (13*8)(%rsp)
	MOVQ	%r15, (14*8)(%rsp)
	// (15*8)(%rsp)	// ureg.type
	// (16*8)(%rsp) // ureg.error

	MOVQ	%rsp, %rsi 				/* Ureg* */
	// system call number is in %rax, as per linux.
	movq	%rax, %rdi
	xorq	%rax, %rax
	pushq	%rax
	popfq				/* clear all flags. is there something else we should clear too? */
	CALL	syscall

	.globl	syscallreturn
syscallreturn:

	// restore from ureg
	MOVQ	(0*8)(%rsp),%rAX
	MOVQ	(1*8)(%rsp),%rBX
	MOVQ	(2*8)(%rsp),%rCX
	MOVQ	(3*8)(%rsp),%rDX
	MOVQ	(4*8)(%rsp),%rSI
	MOVQ	(5*8)(%rsp),%rDI
	MOVQ	(6*8)(%rsp),%rBP
	MOVQ	(7*8)(%rsp),%r8
	MOVQ	(8*8)(%rsp),%r9
	MOVQ	(9*8)(%rsp),%r10
	MOVQ	(10*8)(%rsp),%r11
	MOVQ	(11*8)(%rsp),%r12
	MOVQ	(12*8)(%rsp),%r13
	MOVQ	(13*8)(%rsp),%r14
	MOVQ	(14*8)(%rsp),%r15

	ADDQ	$(17*8), %rsp			/* registers + arguments */

	CLI
	SWAPGS

	MOVQ	0(%rsp), %rCX			/* ip */
	MOVQ	16(%rsp), %r11			/* flags */
	MOVQ	24(%rsp), %rSP			/* sp */

	incq	scx
	sysretq

	.globl sysrforkret
sysrforkret:
	MOVQ	$0, 0(%rsp)
	JMP	syscallreturn
