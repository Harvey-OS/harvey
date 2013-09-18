TEXT	_mulv(SB), $0
	MOVL	r+0(FP), CX
	MOVL	a+4(FP), AX
	MULL	b+12(FP)
	MOVL	AX, 0(CX)
	MOVL	DX, BX
	MOVL	a+4(FP), AX
	MULL	b+16(FP)
	ADDL	AX, BX
	MOVL	a+8(FP), AX
	MULL	b+12(FP)
	ADDL	AX, BX
	MOVL	BX, 4(CX)
	RET

/*
 * _mul64by32(uint64 *r, uint64 a, uint32 b)
 * sets *r = low 64 bits of 96-bit product a*b; returns high 32 bits.
 */
TEXT	_mul64by32(SB), $0
	MOVL	r+0(FP), CX
	MOVL	a+4(FP), AX
	MULL	b+12(FP)
	MOVL	AX, 0(CX)	/* *r = low 32 bits of a*b */
	MOVL	DX, BX		/* BX = high 32 bits of a*b */

	MOVL	a+8(FP), AX
	MULL	b+12(FP)	/* hi = (a>>32) * b */
	ADDL	AX, BX		/* BX += low 32 bits of hi */
	ADCL	$0, DX		/* DX = high 32 bits of hi + carry */
	MOVL	BX, 4(CX)	/* *r |= (high 32 bits of a*b) << 32 */

	MOVL	DX, AX		/* return hi>>32 */
	RET

TEXT	_div64by32(SB), $0
	MOVL	r+12(FP), CX
	MOVL	a+0(FP), AX
	MOVL	a+4(FP), DX
	DIVL	b+8(FP)
	MOVL	DX, 0(CX)
	RET

TEXT	_addv(SB), $0
	MOVL	r+0(FP), CX
	MOVL	a+4(FP), AX
	MOVL	a+8(FP), BX
	ADDL	b+12(FP), AX
	ADCL	b+16(FP), BX
	MOVL	AX, 0(CX)
	MOVL	BX, 4(CX)
	RET
