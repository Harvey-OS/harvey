#define	sp	R1
#define	NOPROF	1
#define	NOOP	BRA	FALSE, 0(R0)

	GLOBL	invA<>(SB),$12
	DATA	invA<>+0(SB)/4, $1.0
	DATA	invA<>+4(SB)/4, $-0.993464052
	DATA	invA<>+8(SB)/4, $0.836601307

/*
 * calling sequence: return a / b
 *	b in 8(sp)
 *	a in 4(sp) ==>
 *	nothing in 0(sp)
 * 	increment sp by 4 & store result
 */
TEXT	_fdiv(SB), NOPROF, $-4
	NOSCHED

	MOVW	R3, (sp)-
	FMOVF	F0, (sp)-
	FMOVF	F1, (sp)-
	FMOVF	F2, (sp)

	ADD	$((3*4)+8), sp, R2	/* R2: denominator */
	FSEED	(R2), F1
	FMOVF	F1, F2
	MOVW	$invA<>(SB), R3		/* R3: constants used in division */

	FMADDN	(R2), F1, (R3)+, F1
	FMUL	(R2)-, F1, F2		/* R2: numerator */
	FMUL	(R2), F1, F0
	FMUL	F1, F1, F1
	FMUL	(R3)+, F1, F1
	NOOP
	FMADD	(R3), F1, F1, F1
	SUB	$8,R3
	NOOP
	FMADD	F2, F1, F2, F2
	FSUB	(R3), F2, F1
	FMADD	F0, F1, F0, F0
	NOOP
	FMOVF	(sp)+, F2
	FMSUB	F0, F1, F0, F0
	FROUND	F0, F0, (R2)

	FMOVF	(sp)+, F1
	FMOVF	(sp)+, F0

	JMP	(R18)
	MOVW	(sp)+, R3

	SCHED
