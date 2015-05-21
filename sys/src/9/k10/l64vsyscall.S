#include "mem.h"
#include "amd64l.h"
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
//	BYTE $0x48; SYSRET			/* SYSRETQ */
/*
 */
.global syscallentry
syscallentry:
1: jmp 1b
	incq	sce
	SWAPGS	
	movq	%gs:0, %r15
	movq	16(%r15), %r15			/* m->proc */
	// NOTE! This assumes sizeof(Label) == 16!
	MOVQ	%rsp, %r14
	MOVQ	16(%r15), %rsp 			/* m->proc->kstack */
	ADDQ	$KSTACK, %rSP
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* old stack segment */
	PUSHQ	%r14				/* old sp */
	PUSHQ	%r11				/* old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	%rCX				/* old ip */

	// NOTE: we need to make this look like uregs. That includes
	// two uint64_ts for type and error. Total is 17.
	SUBQ	$(17*8), %rsp			/* unsaved registers */
	MOVQ	%rAX, (0)(%rsp)
	MOVQ	%rBX, (1*8)(%rsp)
	MOVQ	%rCX, (2*8)(%rsp)
	MOVQ	%rDX, (3*8)(%rsp)
	MOVQ	%rSI, (4*8)(%rsp)
	MOVQ	%rDI, (5*8)(%rsp)
	MOVQ	%rBP, (6*8)(%rsp)
	MOVQ	%r8, (7*8)(%rsp)
	MOVQ	%r9, (8*8)(%rsp)
	MOVQ	%r10, (9*8)(%rsp)
	MOVQ	%r11, (10*8)(%rsp)
	MOVQ	%r12, (11*8)(%rsp)
#if 0
	// don't bother yet.
	MOVW	$SSEL(SiUDS, SsRPL3), (15*8+0)(%rsp)
	MOVW	ES, (15*8+2)(%rsp)
	MOVW	FS, (15*8+4)(%rsp)
	MOVW	GS, (15*8+6)(%rsp)
#endif

	MOVQ	%rsp, %rsi 				/* Ureg* */
	// system call number is in %r9
	movq	%r9, %rdi
	CALL	syscall

	.globl	syscallreturn
syscallreturn:	
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
	MOVQ	0(%rsp), %rAX			/* Ureg.ax */
	MOVQ	(6*8)(%rsp), %rBP		/* Ureg.bp */
_linuxsyscallreturn:
	MOVQ	(14*8)(%rsp),%r15
	MOVQ	(13*8)(%rsp),%r14
	MOVQ	(12*8)(%rsp),%r13
	MOVQ	(11*8)(%rsp),%r12
	MOVQ	(9*8)(%rsp),%r10
	MOVQ	(8*8)(%rsp),%r9
	MOVQ	(7*8)(%rsp),%r8
	MOVQ	(6*8)(%rsp),%rBP
	MOVQ	(5*8)(%rsp),%rDI
	MOVQ	(4*8)(%rsp),%rSI
	MOVQ	(3*8)(%rsp),%rDX
	MOVQ	(1*8)(%rsp),%rBX
	ADDQ	$(17*8), %rsp			/* registers + arguments */

	/* the CLI should maybe be done sooner */
	CLI
	SWAPGS
#if 0
	MOVW	0(%rsp), DS
	MOVW	2(%rsp), ES
	/* This breaks TLS.
	MOVW	4(%rsp), FS
	*/
	MOVW	6(%rsp), GS
#endif	

	MOVQ	0(%rsp), %rCX			/* ip */
	MOVQ	16(%rsp), %r11			/* flags */

	MOVQ	24(%rsp), %rSP			/* sp */

	incq	scx
	sysretq
//	BYTE $0x48; SYSRET			/* SYSRETQ */

	.globl sysrforkret
sysrforkret:	
//	CALL	linuxclonefinish(SB)
	// DEBUG
	MOVQ	$0, %rAX
	JMP	syscallreturn
