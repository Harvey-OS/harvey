/*
 *	ulong
 *	_udiv(ulong num, ulong den)
 *	{
 *		int i;
 *		ulong quo;
 *
 *		if(den == 0)
 *			*(ulong*)-1 = 0;
 *		quo = num;
 *		if(quo > 1<<(32-1))
 *			quo = 1<<(32-1);
 *		for(i=0; den<quo; i++)
 *			den <<= 1;
 *		quo = 0;
 *		for(; i>=0; i--) {
 *			quo <<= 1;
 *			if(num >= den) {
 *				num -= den;
 *				quo |= 1;
 *			}
 *			den >>= 1;
 *		}
 *		return quo::num;
 *	}
 */

#define	NOPROF	1
#define	sp	R1
#define	NOOP	BRA	FALSE, 0(R0)

/*
 * calling sequence:
 *	num: R2
 *	den: R3
 * returns
 *	quo: R2
 *	rem: R3
 * uses R2, R3, R4, R5
 */
TEXT	_udivmod(SB), NOPROF, $-4

	NOSCHED
	CMP	R3, R0
	BRA	NE, nofault
	MOVW	$(1<<31), R5
	MOVW	R0,R2
	JMP	(R6)
	MOVW	R2,R3
nofault:
	CMP	R2, R5
	BRA	LS, upden
	MOVW	R2, R4		/* r2 is quo, r4 is num */
	MOVW	R5, R2
upden:
	CMP	R3, R2
	BRA	CC, doit
	MOVW	$-1, R5		/* r5 is i; -1 since DBRA tests before dec */

updenl:
	SLL	$1, R3
	CMP	R3, R2
	BRA	CS, updenl
	ADD	$1, R5
doit:
	MOVW	R0, R2
divl:
	CMP	R4, R3
	BRA	CS, nosub
	SLL	$1, R2
	SUB	R3, R4
	OR	$1, R2
nosub:
	DBRA	R5, divl
	SRL	$1, R3

	JMP	(R6)
	MOVW	R4, R3		/* rem */
	SCHED

/*
 * calling sequence
 *	num: R2
 *	den: 4(sp)
 * returns
 *	quo: R2
 */
TEXT	_div(SB), NOPROF, $-4
	NOSCHED
	MOVW	R3, (sp)-
	ADD	$8, sp, R3
	MOVW	(R3), R3
	MOVW	R4, (sp)-

	XOR	R2, R3, R4		/* different signs => negate result */

	CMP	R2, R0
	SUB	LT, R2, R0, R2
	CMP	R3, R0
	SUB	LT, R3, R0, R3

	CMP	R4, R0
	BRA	LT, divneg
	MOVW	R6, (sp)-

	CALL	R6, _udivmod(SB)
	MOVW	R5, (sp)

divret:
	MOVW	(sp)+, R5
	MOVW	(sp)+, R6
	MOVW	(sp)+, R4
	JMP	(R18)
	MOVW	(sp), R3

divneg:
	CALL	R6, _udivmod(SB)
	MOVW	R5, (sp)
	JMP	divret
	SUB	R2, R0, R2
	SCHED

/*
 * calling sequence
 *	num: R2
 *	den: 4(sp)
 * returns
 *	quo: R2
 */
TEXT	_divl(SB), NOPROF, $-4
	NOSCHED
	MOVW	R3, (sp)-
	ADD	$8, sp, R3
	MOVW	(R3), R3
	MOVW	R4, (sp)-
	MOVW	R6, (sp)-

	CALL	R6, _udivmod(SB)
	MOVW	R5, (sp)

	JMP	divret+1		/* divret, put the first in the delay slot */
	MOVW	(sp)+, R5
	SCHED

/*
 * calling sequence
 *	num: R2
 *	den: 4(sp)
 * returns
 *	rem: R2
 */
TEXT	_mod(SB), NOPROF, $-4
	NOSCHED
	MOVW	R3, (sp)-
	ADD	$8, sp, R3
	MOVW	(R3), R3
	MOVW	R4, (sp)-

	CMP	R3, R0
	SUB	LT, R3, R0, R3
	CMP	R2, R0		/* num neg => negate result */
	BRA	LT, modneg
	MOVW	R6, (sp)-

	CALL	R6, _udivmod(SB)
	MOVW	R5, (sp)
	JMP	divret
	MOVW	R3, R2

modneg:
	SUB	R2, R0, R2
	CALL	R6, _udivmod(SB)
	MOVW	R5, (sp)
	JMP	divret
	SUB	R3, R0, R2
	SCHED

/*
 * calling sequence
 *	num: R2
 *	den: 4(sp)
 * returns
 *	rem: R2
 */
TEXT	_modl(SB), NOPROF, $-4
	NOSCHED
	MOVW	R3, (sp)-
	ADD	$8, sp, R3
	MOVW	(R3), R3
	MOVW	R4, (sp)-
	MOVW	R6, (sp)-

	CALL	R6, _udivmod(SB)
	MOVW	R5, (sp)

	JMP	divret
	MOVW	R3, R2
	SCHED

/*
 * calling sequence:
 *	arg1 in R2
 *	arg2 in 4(sp) ==> R3  save R4
 *	nothing in 0(sp)
 * 	result in R2
 *
 *	c = b;
 *	if(a & 1) b += c;
 *	a >>= 1;
 *	if(a == 0) goto mulret
 *	c <<= 1;
 */
TEXT	_mul+0(SB), NOPROF, $-4
	NOSCHED
	MOVW	R3, (sp)-
	ADD	$8, sp, R3
	MOVW	(R3), R3
	MOVW	R4, (sp)

	CMP	R2, R3
	BRA	GE, domul
	MOVW	R2, R4

	MOVW	R3, R4
	MOVW	R2, R3

domul:
	MOVW	R0, R2

	BIT	R3, $1		/* bit test ordered as compare */
	ADD	NE, R4, R2
	SRL	$1, R3
	BRA	EQ, mulret
	SLL	$1, R4		/* ok if we do this an extra time */

mulloop:
	BIT	R3, $1
	ADD	NE, R4, R2
	SRL	$1, R3
	BRA	NE, mulloop
	SLL	$1, R4		/* ok if we do this an extra time */

mulret:
	MOVW	(sp)+, R4
	JMP	(R18)
	MOVW	(sp), R3
	SCHED
