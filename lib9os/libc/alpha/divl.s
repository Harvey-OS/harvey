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

/*
 * calling sequence:
 *	num: 8(R30)
 *	den: 12(R30)
 * returns
 *	quo: 8(R30)
 *	rem: 12(R30)
 */
TEXT	_udivmodl(SB), NOPROF, $-8

	MOVQ	$-1, R11
	SLLQ	$31, R11			/* (1<<31) in canonical form */
	MOVL	8(R30), R23	/* numerator */
	MOVL	12(R30), R10	/* denominator */
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
	SLLL	$1, R10
	ADDQ	$1, R11
	JMP	udm38
udm54:
	MOVQ	R31, R12
udm58:
	BLT	R11, udm8c
	SLLL		$1, R12
	CMPUGE	R23, R10, R24
	BEQ		R24, udm7c
	SUBL		R10, R23
	OR		$1, R12
udm7c:
	SRLL		$1, R10
	SUBQ	$1, R11
	JMP	udm58
udm8c:
	MOVL	R12, 8(R30)	/* quotient */
	MOVL	R23, 12(R30)	/* remainder */
	RET

/*
 * save working registers
 * and bring in num/den parameters
 */
TEXT	_unsargl(SB), NOPROF, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVL	R27, 8(R30)
	MOVL	72(R30), R27
	MOVL	R27, 12(R30)

	RET

/*
 * save working registers
 * and bring in absolute value
 * of num/den parameters
 */
TEXT	_absargl(SB), NOPROF, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVL	R27, 64(R30)
	BGE	R27, ab1
	SUBL	R27, R31, R27
ab1:
	MOVL	R27, 8(R30)	/* numerator */

	MOVL	72(R30), R27
	BGE	R27, ab2
	SUBL	R27, R31, R27
ab2:
	MOVL	R27, 12(R30)	/* denominator */
	RET

/*
 * restore registers and
 * return to original caller
 * answer is in R27
 */
TEXT	_retargl(SB), NOPROF, $-8
	MOVQ	24(R30), R10
	MOVQ	32(R30), R11
	MOVQ	40(R30), R12
	MOVQ	48(R30), R23
	MOVQ	56(R30), R24
	MOVL	0(R30), R26

	ADDQ	$64, R30
	RET			/* back to main sequence */

TEXT	_divl(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	_absargl(SB)
	JSR	_udivmodl(SB)
	MOVL	8(R30), R27

	MOVL	64(R30), R10	/* clean up the sign */
	MOVL	72(R30), R11
	XOR	R11, R10
	BGE	R10, div1
	SUBL	R27, R31, R27
div1:

	JSR	_retargl(SB)
	RET			/* not executed */

TEXT	_divlu(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	_unsargl(SB)
	JSR	_udivmodl(SB)
	MOVL	8(R30), R27

	JSR	_retargl(SB)
	RET			/* not executed */

TEXT	_modl(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	_absargl(SB)
	JSR	_udivmodl(SB)
	MOVL	12(R30), R27

	MOVL	64(R30), R10	/* clean up the sign */
	BGE	R10, div2
	SUBL	R27, R31, R27
div2:

	JSR	_retargl(SB)
	RET			/* not executed */

TEXT	_modlu(SB), NOPROF, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	_unsargl(SB)
	JSR	_udivmodl(SB)
	MOVL	12(R30), R27

	JSR	_retargl(SB)
	RET			/* not executed */

