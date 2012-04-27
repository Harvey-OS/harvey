/*
 *	mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
 *
 *		sum[0:alen] = a[0:alen-1] + b[0:blen-1]
 *
 *	prereq: alen >= blen, sum has room for alen+1 digits
 */
TEXT	mpvecadd(SB),$0

	MOVL	alen+8(FP),DX
	MOVL	blen+24(FP),CX
/*	MOVL	a+0(FP),SI */
	MOVQ	RARG, SI
	MOVQ	b+16(FP),BX
	SUBL	CX,DX
	MOVQ	sum+32(FP),DI
	XORL	BP,BP			/* this also sets carry to 0 */

	/* skip addition if b is zero */
	TESTL	CX,CX
	JZ	_add1

	/* sum[0:blen-1],carry = a[0:blen-1] + b[0:blen-1] */
_addloop1:
	MOVL	(SI)(BP*4), AX
	ADCL	(BX)(BP*4), AX
	MOVL	AX,(DI)(BP*4)
	INCL	BP
	LOOP	_addloop1

_add1:
	/* jump if alen > blen */
	INCL	DX
	MOVL	DX,CX
	LOOP	_addloop2

	/* sum[alen] = carry */
_addend:
	JC	_addcarry
	MOVL	$0,(DI)(BP*4)
	RET
_addcarry:
	MOVL	$1,(DI)(BP*4)
	RET

	/* sum[blen:alen-1],carry = a[blen:alen-1] + 0 */
_addloop2:
	MOVL	(SI)(BP*4),AX
	ADCL	$0,AX
	MOVL	AX,(DI)(BP*4)
	INCL	BP
	LOOP	_addloop2
	JMP	_addend

