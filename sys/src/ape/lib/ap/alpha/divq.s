/*
 *	uvlong
 *	_udiv(uvlong num, uvlong den)
 *	{
 *		int i;
 *		uvlong quo;
 *
 *		if(den == 0)
 *			*(ulong*)-1 = 0;
 *		quo = num;
 *		if(quo > 1<<(64-1))
 *			quo = 1<<(64-1);
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

/*
 * calling sequence:
 *	num: 8(R30)
 *	den: 16(R30)
 * returns
 *	quo: 8(R30)
 *	rem: 16(R30)
 */
TEXT	_udivmodq(SB), NOPROF, $-8

	MOVQ	$1, R11
	SLLQ	$63, R11
	MOVQ	8(R30), R23	/* numerator */
	MOVQ	16(R30), R10	/* denominator */
	BNE	R10, udm20
	MOVQ	R31, -1(R31)	/* fault -- divide by zero; todo: use gentrap? */
udm20:
	MOVQ	R23, R12
	BGE	R12, udm34
	MOVQ	R11, R12
udm34:
	MOVQ	R31, R11
udm38:
	CMPUGE	R10, R12, R24
	BNE	R24, udm54
	SLLQ	$1, R10
	ADDQ	$1, R11
	JMP	udm38
udm54:
	MOVQ	R31, R12
udm58:
	BLT	R11, udm8c
	SLLQ	$1, R12
	CMPUGE	R23, R10, R24
	BEQ	R24, udm7c
	SUBQ	R10, R23
	OR	$1, R12
udm7c:
	SRLQ	$1, R10
	SUBQ	$1, R11
	JMP	udm58
udm8c:
	MOVQ	R12, 8(R30)	/* quotient */
	MOVQ	R23, 16(R30)	/* remainder */
	RET

/*
 * save working registers
 * and bring in num/den parameters
 */
TEXT	_unsargq(SB), NOPROF, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVQ	R27, 8(R30)
	MOVQ	72(R30), R27
	MOVQ	R27, 16(R30)

MOVQ	(R30), R10	/* debug */
	RET

/*
 * save working registers
 * and bring in absolute value
 * of num/den parameters
 */
TEXT	_absargq(SB), NOPROF, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVQ	R27, 64(R30)
	BGE	R27, ab1
	SUBQ	R27, R31, R27
ab1:
	MOVQ	R27, 8(R30)	/* numerator */

	MOVQ	72(R30), R27
	BGE	R27, ab2
	SUBQ	R27, R31, R27
ab2:
	MOVQ	R27, 16(R30)	/* denominator */
MOVQ	(R30), R10	/* debug */
	RET

/*
 * restore registers and
 * return to original caller
 * answer is in R27
 */
TEXT	_retargq(SB), NOPROF, $-8
	MOVQ	24(R30), R10
	MOVQ	32(R30), R11
	MOVQ	40(R30), R12
	MOVQ	48(R30), R23
	MOVQ	56(R30), R24
	MOVL	0(R30), R26

	ADDQ	$64, R30
	RET			/* back to main sequence */

TEXT	_divq(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	_absargq(SB)
	JSR	_udivmodq(SB)
	MOVQ	8(R30), R27

	MOVQ	64(R30), R10	/* clean up the sign */
	MOVQ	72(R30), R11
	XOR	R11, R10
	BGE	R10, div1
	SUBQ	R27, R31, R27
div1:

	JSR	_retargq(SB)
	RET			/* not executed */

TEXT	_divqu(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	_unsargq(SB)
	JSR	_udivmodq(SB)
	MOVQ	8(R30), R27

	JSR	_retargq(SB)
	RET			/* not executed */

TEXT	_modq(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	_absargq(SB)
	JSR	_udivmodq(SB)
	MOVQ	16(R30), R27

	MOVQ	64(R30), R10	/* clean up the sign */
	BGE	R10, div2
	SUBQ	R27, R31, R27
div2:

	JSR	_retargq(SB)
	RET			/* not executed */

TEXT	_modqu(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	_unsargq(SB)
	JSR	_udivmodq(SB)
	MOVQ	16(R30), R27

	JSR	_retargq(SB)
	RET			/* not executed */

