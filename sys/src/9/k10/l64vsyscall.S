#include "mem.h"
#include "amd64l.h"
#ifndef __ASSEMBLER__
#define __ASSEMBLER__
#endif


/*
 * starting the user program up. First time.
 */
	.globl touser
touser:	
	CLI
	SWAPGS
#if 0
	// we should be able to skip this step. We'll see.
	MOVQ	$SSEL(SiUDS, SsRPL3), AX
	MOVW	AX, DS
	MOVW	AX, ES
	/*
	 * FS is used for TLS. Don't touch!
	MOVW	AX, FS
 	 */
	MOVW	AX, GS
#endif
	// Hmm. Assuumes many things, eh? Assumes plan 9 a.out
	// format. This will fail.
	MOVQ	$(UTZERO+0x28), %rcx		/* ip */
	MOVQ	$If, %R11			/* flags */
#warning "remove this line when init works better, it disables interrupts for user processes"
	MOVQ	$0, %R11			/* flags */

	MOVW	$0xebfe,	0(%rcx)
	MOVQ	%RDI, %RSP			/* sp */

	sysretq
//	BYTE $0x48; SYSRET			/* SYSRETQ */
/*
 */
.global syscallentry
syscallentry:
	SWAPGS	
	movq	%gs:0, %r15
	movq	16(%r15), %r15			/* m->proc */
	// NOTE! This assumes sizeof(Label) == 16!
	MOVQ	16(%r15), %rsp 			/* m->proc->kstack */
	ADDQ	$KSTACK, %rSP
	PUSHQ	$SSEL(SiUDS, SsRPL3)		/* old stack segment */
	PUSHQ	%r13				/* old sp */
	PUSHQ	%r11				/* old flags */
	PUSHQ	$SSEL(SiUCS, SsRPL3)		/* old code segment */
	PUSHQ	%rCX				/* old ip */

	SUBQ	$(18*8), %rsp			/* unsaved registers */
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

	PUSHQ	%rsp				/* Ureg* */
//	PUSHQ	%RDI				/* system call number */
	// system call # is in %rdi so no need to push it.
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
	MOVQ	16(%rsp), %rAX			/* Ureg.ax */
	MOVQ	(16+6*8)(%rsp), %rBP		/* Ureg.bp */
_linuxsyscallreturn:
	/* aka RMACH and RUSER */
	MOVQ	(16+14*8)(%rsp),%r15
	MOVQ	(16+13*8)(%rsp),%r14
	MOVQ	(16+12*8)(%rsp),%r13
	MOVQ	(16+11*8)(%rsp),%r12
	MOVQ	(16+9*8)(%rsp),%r10
	MOVQ	(16+8*8)(%rsp),%r9
	MOVQ	(16+7*8)(%rsp),%r8
	MOVQ	(16+6*8)(%rsp),%rBP
	MOVQ	(16+5*8)(%rsp),%rDI
	MOVQ	(16+4*8)(%rsp),%rSI
	MOVQ	(16+3*8)(%rsp),%rDX
	MOVQ	(16+1*8)(%rsp),%rBX
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

	MOVQ	24(%rsp), %rCX			/* ip */
	MOVQ	40(%rsp), %r11			/* flags */

	MOVQ	48(%rsp), %rSP			/* sp */

	sysretq
//	BYTE $0x48; SYSRET			/* SYSRETQ */

	.globl sysrforkret
sysrforkret:	
//	CALL	linuxclonefinish(SB)
	MOVQ	$0, %rAX
	JMP	syscallreturn
