TEXT	mpdigdiv(SB),$0

	MOVL	dividend+0(FP),BX
	MOVL	0(BX),AX
	MOVL	4(BX),DX
	MOVL	divisor+4(FP),BX
	MOVL	quotient+8(FP),BP
	XORL	CX,CX
	CMPL	DX,BX		/* dividend >= 2^32 * divisor */
	JHS	_divovfl
	CMPL	BX,CX		/* divisor == 0 */
	JE	_divovfl
	DIVL	BX		/* AX = DX:AX/BX */
	MOVL	AX,0(BP)
	RET

	/* return all 1's */
_divovfl:
	NOTL	CX
	MOVL	CX,0(BP)
	RET
