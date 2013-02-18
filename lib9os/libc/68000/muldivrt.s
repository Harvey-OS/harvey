/*
 * calls _divul with
 * absolute value arguments
 */
TEXT	_divsl(SB), $0
	MOVL	R0, TOS

	MOVL	b+4(FP), R0
	BPL	y1
	NEGL	R0
	MOVL	R0, TOS

	MOVL	a+0(FP), R0
	BPL	y3
	NEGL	R0
	MOVL	R0, TOS

	/* neg/neg */
	JSR	_divul(SB)
	MOVL	TOS, R0
	MOVL	R0, a+0(FP)
	MOVL	TOS, R0
	NEGL	R0
	MOVL	R0, b+4(FP)
	MOVL	TOS, R0
	RTS

y1:	MOVL	R0, TOS

	MOVL	a+0(FP), R0
	BPL	y2
	NEGL	R0
	MOVL	R0, TOS

	/* neg/pos */
	JSR	_divul(SB)
	MOVL	TOS, R0
	NEGL	R0
	MOVL	R0, a+0(FP)
	MOVL	TOS, R0
	NEGL	R0
	MOVL	R0, b+4(FP)
	MOVL	TOS, R0
	RTS

y2:	MOVL	R0, TOS

	/* pos/pos */
	JSR	_divul(SB)
	MOVL	TOS, R0
	MOVL	R0, a+0(FP)
	MOVL	TOS, R0
	MOVL	R0, b+4(FP)
	MOVL	TOS, R0
	RTS

y3:	MOVL	R0, TOS

	/* pos/neg */
	JSR	_divul(SB)
	MOVL	TOS, R0
	NEGL	R0
	MOVL	R0, a+0(FP)
	MOVL	TOS, R0
	MOVL	R0, b+4(FP)
	MOVL	TOS, R0
	RTS

/*
 *	for(i=1;; i++) {
 *		if(den & (1<<31))
 *			break;
 *		den <<= 1;
 *	}
 *
 *	for(; i; i--) {
 *		quo <<= 1;
 *		if(num >= den) {
 *			num -= den;
 *			quo |= 1;
 *		}
 *		den >>= 1;
 *	}
 */
TEXT	_divul(SB), $0
	MOVL	R0, TOS	/* i */
	MOVL	R1, TOS	/* num */
	MOVL	R2, TOS	/* den */
	MOVL	R3, TOS	/* quo */

	MOVL	$0, R0
	MOVL	$0, R3
	MOVL	a+0(FP), R1
	MOVL	b+4(FP), R2
	BEQ	xout
	BMI	x1

	ADDL	$1, R0
	LSLL	$1, R2
	BPL	-2(PC)

x1:	LSLL	$1, R3
	CMPL	R1, R2
	BCS	3(PC)
	SUBL	R2, R1
	ORL	$1, R3
	LSRL	$1, R2
	DBMI	R0, x1

	MOVL	R3, a+0(FP)
	MOVL	R1, b+4(FP)

xout:
	MOVL	TOS, R3
	MOVL	TOS, R2
	MOVL	TOS, R1
	MOVL	TOS, R0
	RTS

/*
 *	x = 0;
 *	for(i=0; i<32; i++) {
 *		if(a & 1)
 *			x += b;
 *		a >>= 1;
 *		b <<= 1;
 *	}
 *	a = x;
 */
TEXT	_mull(SB), $0
	MOVL	R0, TOS	/* i */
	MOVL	R1, TOS	/* a */
	MOVL	R2, TOS	/* b */
	MOVL	R3, TOS	/* x */

	MOVL	a+0(FP), R1
	MOVL	b+4(FP), R2
	MOVL	$32, R0
	CLRL	R3

z1:	ROTRL	$1, R1
	BCC	2(PC)
	ADDL	R2, R3
	LSLL	$1, R2
	DBEQ	R0, z1

	MOVL	R3, b+4(FP)
	MOVL	TOS, R3
	MOVL	TOS, R2
	MOVL	TOS, R1
	MOVL	TOS, R0
	RTS

TEXT	_ccr(SB), $0
	PEA	(A0)
	SUBL	A0, A0

	BCC	2(PC)
	LEA	1(A0), A0

	BVC	2(PC)
	LEA	2(A0), A0

	BNE	2(PC)
	LEA	4(A0), A0

	BPL	2(PC)
	LEA	8(A0), A0

	MOVW	A0, a+0(FP)
	MOVL	TOS, A0
	RTS
