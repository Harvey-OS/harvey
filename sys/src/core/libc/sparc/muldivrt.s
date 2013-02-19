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
 *	num: 4(R1)
 *	den: 8(R1)
 * returns
 *	quo: 4(R1)
 *	rem: 8(R1)
 */
TEXT	_udivmod(SB), NOPROF, $-4

	MOVW	$(1<<31), R11
	MOVW	4(R1), R13	/* numerator */
	MOVW	8(R1), R10	/* denominator */
	CMP	R10, R0
	BNE	udm20
	MOVW	R0, -1(R0)	/* fault -- divide by zero */
udm20:
	MOVW	R13, R12
	CMP	R13, R11
	BLEU	udm34
	MOVW	R11, R12
udm34:
	MOVW	R0, R11
udm38:
	CMP	R10, R12
	BCC	udm54
	SLL	$1, R10
	ADD	$1, R11
	BA	udm38
udm54:
	MOVW	R0, R12
udm58:
	CMP	R11, R0
	BL	udm8c
	SLL	$1, R12
	CMP	R13, R10
	BCS	udm7c
	SUB	R10, R13
	OR	$1, R12
udm7c:
	SRL	$1, R10
	SUB	$1, R11
	BA	udm58
udm8c:
	MOVW	R12, 4(R1)	/* quotent */
	MOVW	R13, 8(R1)	/* remainder */
	JMPL	8(R15)

/*
 * save working registers
 * and bring in num/den parameters
 */
TEXT	_unsarg(SB), NOPROF, $-4
	MOVW	R10, 12(R1)
	MOVW	R11, 16(R1)
	MOVW	R12, 20(R1)
	MOVW	R13, 24(R1)

	MOVW	R14, 4(R1)
	MOVW	32(R1), R14
	MOVW	R14, 8(R1)

	JMPL	8(R15)

/*
 * save working registers
 * and bring in absolute value
 * of num/den parameters
 */
TEXT	_absarg(SB), NOPROF, $-4
	MOVW	R10, 12(R1)
	MOVW	R11, 16(R1)
	MOVW	R12, 20(R1)
	MOVW	R13, 24(R1)

	MOVW	R14, 28(R1)
	CMP	R14, R0
	BGE	ab1
	SUB	R14, R0, R14
ab1:
	MOVW	R14, 4(R1)	/* numerator */

	MOVW	32(R1), R14
	CMP	R14, R0
	BGE	ab2
	SUB	R14, R0, R14
ab2:
	MOVW	R14, 8(R1)	/* denominator */
	JMPL	8(R15)

/*
 * restore registers and
 * return to original caller
 * answer is in R14
 */
TEXT	_retarg(SB), NOPROF, $-4
	MOVW	12(R1), R10
	MOVW	16(R1), R11
	MOVW	20(R1), R12
	MOVW	24(R1), R13
	MOVW	0(R1), R15

	ADD	$28, R1
	JMP	8(R15)		/* back to main sequence */

/*
 * calling sequence
 *	num: R14
 *	den: 8(R1)
 * returns
 *	quo: R14
 */
TEXT	_div(SB), NOPROF, $-4
	SUB	$28, R1		/* 4 reg save, 2 parameters, link */
	MOVW	R15, 0(R1)

	JMPL	_absarg(SB)
	JMPL	_udivmod(SB)
	MOVW	4(R1), R14

	MOVW	28(R1), R10	/* clean up the sign */
	MOVW	32(R1), R11
	XORCC	R11, R10, R0
	BGE	div1
	SUB	R14, R0, R14
div1:

	JMPL	_retarg(SB)
	JMP	8(R15)		/* not executed */

/*
 * calling sequence
 *	num: R14
 *	den: 8(R1)
 * returns
 *	quo: R14
 */
TEXT	_divl(SB), NOPROF, $-4
	SUB	$((4+2+1)*4), R1	/* 4 reg save, 2 parameters, link */
	MOVW	R15, 0(R1)

	JMPL	_unsarg(SB)
	JMPL	_udivmod(SB)
	MOVW	4(R1), R14

	JMPL	_retarg(SB)
	JMP	8(R15)		/* not executed */

/*
 * calling sequence
 *	num: R14
 *	den: 8(R1)
 * returns
 *	rem: R14
 */
TEXT	_mod(SB), NOPROF, $-4
	SUB	$28, R1		/* 4 reg save, 2 parameters, link */

	MOVW	R15, 0(R1)
	JMPL	_absarg(SB)
	JMPL	_udivmod(SB)
	MOVW	8(R1), R14

	MOVW	28(R1), R10	/* clean up the sign */
	CMP	R10, R0
	BGE	mod1
	SUB	R14, R0, R14
mod1:

	JMPL	_retarg(SB)
	JMP	8(R15)		/* not executed */

/*
 * calling sequence
 *	num: R14
 *	den: 8(R1)
 * returns
 *	rem: R14
 */
TEXT	_modl(SB), NOPROF, $-4
	SUB	$28, R1		/* 4 reg save, 2 parameters, link */


	MOVW	R15, 0(R1)
	JMPL	_unsarg(SB)
	JMPL	_udivmod(SB)
	MOVW	8(R1), R14

	JMPL	_retarg(SB)
	JMP	8(R15)		/* not executed */

/*
 * special calling sequence:
 *	arg1 in R14
 *	arg2 in 4(R1),    will save R9
 *	nothing in 0(R1), will save R8
 * 	result in R14
 */
TEXT	_mul+0(SB), NOPROF, $-4

	/*
	 * exchange stack and registers
	 */
	MOVW	R8, 0(R1)
	MOVW	4(R1), R8
	MOVW	R9, 4(R1)

	CMP	R14, R8
	BLE	mul1
	MOVW	R14, R9
	MOVW	R8, R14
	MOVW	R9, R8
mul1:
	MOVW	R14, Y
	ANDNCC	$0xFFF, R14, R0
	BE	mul_shortway
	ANDCC	R0, R0, R9	/* zero partial product and clear N and V cond's */

	/* long multiply */
	MULSCC	R8, R9, R9		/* 0 */
	MULSCC	R8, R9, R9		/* 1 */
	MULSCC	R8, R9, R9		/* 2 */
	MULSCC	R8, R9, R9		/* 3 */
	MULSCC	R8, R9, R9		/* 4 */
	MULSCC	R8, R9, R9		/* 5 */
	MULSCC	R8, R9, R9		/* 6 */
	MULSCC	R8, R9, R9		/* 7 */
	MULSCC	R8, R9, R9		/* 8 */
	MULSCC	R8, R9, R9		/* 9 */
	MULSCC	R8, R9, R9		/* 10 */
	MULSCC	R8, R9, R9		/* 11 */
	MULSCC	R8, R9, R9		/* 12 */
	MULSCC	R8, R9, R9		/* 13 */
	MULSCC	R8, R9, R9		/* 14 */
	MULSCC	R8, R9, R9		/* 15 */
	MULSCC	R8, R9, R9		/* 16 */
	MULSCC	R8, R9, R9		/* 17 */
	MULSCC	R8, R9, R9		/* 18 */
	MULSCC	R8, R9, R9		/* 19 */
	MULSCC	R8, R9, R9		/* 20 */
	MULSCC	R8, R9, R9		/* 21 */
	MULSCC	R8, R9, R9		/* 22 */
	MULSCC	R8, R9, R9		/* 23 */
	MULSCC	R8, R9, R9		/* 24 */
	MULSCC	R8, R9, R9		/* 25 */
	MULSCC	R8, R9, R9		/* 26 */
	MULSCC	R8, R9, R9		/* 27 */
	MULSCC	R8, R9, R9		/* 28 */
	MULSCC	R8, R9, R9		/* 29 */
	MULSCC	R8, R9, R9		/* 30 */
	MULSCC	R8, R9, R9		/* 31 */
	MULSCC	R0, R9, R9		/* 32; shift only */

	MOVW	Y, R14			/* get low part */
	BA	mul_return

mul_shortway:
	ANDCC	R0, R0, R9	/* zero partial product and clear N and V cond's */
	MULSCC	R8, R9, R9		/*  0 */
	MULSCC	R8, R9, R9		/*  1 */
	MULSCC	R8, R9, R9		/*  2 */
	MULSCC	R8, R9, R9		/*  3 */
	MULSCC	R8, R9, R9		/*  4 */
	MULSCC	R8, R9, R9		/*  5 */
	MULSCC	R8, R9, R9		/*  6 */
	MULSCC	R8, R9, R9		/*  7 */
	MULSCC	R8, R9, R9		/*  8 */
	MULSCC	R8, R9, R9		/*  9 */
	MULSCC	R8, R9, R9		/* 10 */
	MULSCC	R8, R9, R9		/* 11 */
	MULSCC	R0, R9, R9		/* 12; shift only */

	MOVW	Y, R8
	SLL	$12, R9
	SRL	$20, R8
	OR	R8, R9, R14

mul_return:
	MOVW	0(R1), R8
	MOVW	4(R1), R9
	JMP	8(R15)
