TEXT	mpdigdiv(SB),$0

/*	MOVL	dividend+0(FP),BX */
	MOVL	0(RARG),AX
	MOVL	4(RARG),DX
	MOVL	divisor+8(FP),BX
	MOVQ	quotient+16(FP),DI
	XORL	CX,CX
	CMPL	DX,BX		/* dividend >= 2^32 * divisor */
	JHS	_divovfl
	CMPL	BX,CX		/* divisor == 0 */
	JE	_divovfl
	DIVL	BX		/* AX = DX:AX/BX */
	MOVL	AX,0(DI)
	RET

	/* return all 1's */
_divovfl:
	NOTL	CX
	MOVL	CX,0(DI)
	RET
