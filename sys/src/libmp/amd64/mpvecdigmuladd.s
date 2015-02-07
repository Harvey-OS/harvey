/*
 *	mpvecdigmul(mpdigit *b, int n, mpdigit m, mpdigit *p)
 *
 *	p += b*m
 *
 *	each step look like:
 *		hi,lo = m*b[i]
 *		lo += oldhi + carry
 *		hi += carry
 *		p[i] += lo
 *		oldhi = hi
 *
 *	the registers are:
 *		hi = DX		- constrained by hardware
 *		lo = AX		- constrained by hardware
 *		b+n = SI	- can't be BP
 *		p+n = DI	- can't be BP
 *		i-n = BP
 *		m = BX
 *		oldhi = CX
 *		
 */
TEXT	mpvecdigmuladd(SB),$0

/*	MOVQ	b+0(FP),SI	*/
	MOVQ	RARG,SI
	MOVL	n+8(FP),CX
	MOVL	m+16(FP),BX
	MOVQ	p+24(FP),DI
	MOVL	CX,BP
	NEGQ	BP		/* BP = -n */
	SHLL	$2,CX
	ADDQ	CX,SI		/* SI = b + n */
	ADDQ	CX,DI		/* DI = p + n */
	XORL	CX,CX
_muladdloop:
	MOVL	(SI)(BP*4),AX	/* lo = b[i] */
	MULL	BX		/* hi, lo = b[i] * m */
	ADDL	CX,AX		/* lo += oldhi */
	JCC	_muladdnocarry1
	INCL	DX		/* hi += carry */
_muladdnocarry1:
	ADDL	AX,(DI)(BP*4)	/* p[i] += lo */
	JCC	_muladdnocarry2
	INCL	DX		/* hi += carry */
_muladdnocarry2:
	MOVL	DX,CX		/* oldhi = hi */
	INCQ	BP		/* i++ */
	JNZ	_muladdloop
	XORL	AX,AX
	ADDL	CX,(DI)(BP*4)	/* p[n] + oldhi */
	ADCL	AX,AX		/* return carry out of p[n] */
	RET
