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
 *		b = SI		- can't be BP
 *		p = DI		- can't be BP
 *		i = BP
 *		n = CX		- constrained by LOOP instr
 *		m = BX
 *		oldhi = EX
 *		
 */
TEXT	mpvecdigmuladd(SB),$0

	MOVL	b+0(FP),SI
	MOVL	n+4(FP),CX
	MOVL	m+8(FP),BX
	MOVL	p+12(FP),DI
	XORL	BP,BP
	PUSHL	BP
_muladdloop:
	MOVL	(SI)(BP*4),AX	/* lo = b[i] */
	MULL	BX		/* hi, lo = b[i] * m */
	ADDL	0(SP),AX	/* lo += oldhi */
	JCC	_muladdnocarry1
	INCL	DX		/* hi += carry */
_muladdnocarry1:
	ADDL	AX,(DI)(BP*4)	/* p[i] += lo */
	JCC	_muladdnocarry2
	INCL	DX		/* hi += carry */
_muladdnocarry2:
	MOVL	DX,0(SP)	/* oldhi = hi */
	INCL	BP		/* i++ */
	LOOP	_muladdloop
	MOVL	0(SP),AX
	ADDL	AX,(DI)(BP*4)
	POPL	AX
	RET
