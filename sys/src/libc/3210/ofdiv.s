#define	NOPROF	1
#define	sp	R1
#define	NOOP	BRA	FALSE, 0(R0)

/*
 * floating x/y using newton's method for approximating the recip
float
fdiv(float x, float y)
{
	double r;
	int i;

	r = seed(y);
	r = r * (2. - r * y);
	r = r * (2. - r * y);
	r = r * (2. - r * y);
	r = r * (2. - r * y);
	return r * x;
}
 */
/*
 * calling sequence: return a / b
 *	b in 8(sp)
 *	a in 4(sp) ==>
 *	nothing in 0(sp)
 * 	increment sp by 4 & store result
 */

TEXT	_fdiv(SB), NOPROF, $-4
	NOSCHED
	FMOVF	F0, (sp)-
	FMOVF	F1, (sp)-
	FMOVF	F2, (sp)-
	FMOVF	F3, (sp)

	ADD	$20, sp, R2		/* (R2) is b */
	FMOVF	(R2), F2
	FSEED	(R2), F0
	FMOVF	$2.0, F3		/* modifies R2; takes 2 ops */

	FMSUB	F2, F0, F3, F1
	NOOP
	NOOP
	FMUL	F1, F0, F0
	NOOP
	NOOP
	FMSUB	F2, F0, F3, F1
	NOOP
	NOOP
	FMUL	F1, F0, F0
	NOOP
	NOOP
	FMSUB	F2, F0, F3, F1
	NOOP
	NOOP
	FMUL	F1, F0, F0
	NOOP
	NOOP
	FMSUB	F2, F0, F3, F1
	NOOP
	NOOP
	FMUL	F1, F0, F0		/* w ~= 1/b */
	NOOP
	ADD	$16, sp, R2		/* (R2) is a */

	FMUL	(R2), F0, F1		/* c1 ~= a / b */
	NOOP
	NOOP
	FROUND	F1, F1			/* correctly round the result */
	NOOP
	NOOP
	FMSUB	F1, F2, (R2), F2	/* t = a - b * c1 */
	NOOP
	FMOVF	(sp)+, F3
	FMADD	F2, F0, F1, F0		/* c1 += t * w */
	FMOVF	(sp)+, F2
	FMOVF	(sp)+, F1
	NOOP
	FROUND	F0, F0, (R2)		/* correctly round the result */

	FMOVF	(sp)+, F0
	JMP	(R18)
	NOOP
	SCHED
