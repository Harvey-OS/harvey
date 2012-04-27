#include "mem.h"
#include "arm.h"

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
//	MOVM.S.IA.W (R13), [R13]
	MOVM.S	(R13), [R13]
	ADD	$4, R13

	/* set up a PSR for user level */
	MOVW	$(PsrMusr), R0
	MOVW	R0, SPSR

	/* save the PC on the stack */
	MOVW	$(UTZERO+0x20), R0
	MOVM.DB.W [R0], (R13)

	/*
	 * note that 5a's RFE is not the v6 arch. instruction (0xe89d0a00,
	 * I think), which loads CPSR from the word after the PC at (R13),
	 * but rather the pre-v6 simulation `MOVM.IA.S.W (R13), [R15]'
	 * (0xe8fd8000 since MOVM is LDM in this case), which loads CPSR
	 * not from memory but from SPSR due to `.S'.
	 */
	RFE

/*
 *  here to jump to a newly forked process
 */
TEXT forkret(SB), 1, $-4
	ADD	$(4*15), R13		/* make r13 point to ureg->type */
	MOVW	8(R13), R14		/* restore link */
	MOVW	4(R13), R0		/* restore SPSR */
	MOVW	R0, SPSR		/* ... */
	MOVM.DB.S (R13), [R0-R14]	/* restore registers */
	ADD	$8, R13			/* pop past ureg->{type+psr} */
	RFE				/* MOVM.IA.S.W (R13), [R15] */
