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
	MOVM.DB.W [R0], (R13)
	/* avoid the ambiguity described in notes/movm.w. */
	MOVM.S	(R13), [R13]
	ADD	$4, R13			/* pop new user SP */

	/* set up a PSR for user level */
	MOVW	$(PsrMusr), R0
	MOVW	R0, SPSR

	/* push new user PSR */
	MOVM.DB.W [R0], (R13)

	/* push the new user PC on the stack */
	MOVW	$(UTZERO+0x20), R0
	MOVM.DB.W [R0], (R13)

	RFEV7W(13)

/*
 *  here to jump to a newly forked process
 */
TEXT forkret(SB), 1, $-4
	ADD	$(4*NREGS), R13		/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	B	rfue(SB)
