#include "arm.s"

/*
 *  This is the first jump from kernel to user mode.
 *  Fake a return from interrupt.
 *
 *  Enter with R0 containing the user stack pointer.
 *  UTZERO + 0x20 is always the entry point.
 *
 */
TEXT touser(SB), 1, $-4
	/* store the user stack pointer into the USR_r13 */
	MOVM.DB.W [R0], (SP)
	/* avoid the ambiguity described in notes/movm.w. */
	MOVM.S	(SP), [SP]
	ADD	$BY2WD, SP		/* pop new user SP */

	/* set up a PSR for user level */
	MOVW	$(PsrMusr), R0
	MOVW	R0, SPSR

	/* push new user PSR */
	MOVM.DB.W [R0], (SP)

	/* push the new user PC on the stack */
	MOVW	$(UTZERO+0x20), R0
	MOVM.DB.W [R0], (SP)

	RFEV7W(13)

/*
 *  here to jump to a newly forked process
 */
TEXT forkret(SB), 1, $-4
	ADD	$(BY2WD*NREGS), SP	/* make r13 point to ureg->type */
	MOVW	(2*BY2WD)(SP), R14	/* restore link */
	MOVW	BY2WD(SP), R0		/* restore SPSR */
	B	rfue(SB)
