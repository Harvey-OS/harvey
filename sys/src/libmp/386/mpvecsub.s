/*
 *	mpvecsub(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *diff)
 *
 *		diff[0:alen-1] = a[0:alen-1] - b[0:blen-1]
 *
 *	prereq: alen >= blen, diff has room for alen digits
 */
TEXT	mpvecsub(SB),$0

	MOVL	a+0(FP),SI
	MOVL	b+8(FP),BX
	MOVL	alen+4(FP),DX
	MOVL	blen+12(FP),CX
	MOVL	diff+16(FP),DI
	SUBL	CX,DX
	XORL	BP,BP			/* this also sets carry to 0 */

	/* skip subraction if b is zero */
	TESTL	CX,CX
	JZ	_sub1

	/* diff[0:blen-1],borrow = a[0:blen-1] - b[0:blen-1] */
_subloop1:
	MOVL	(SI)(BP*4),AX
	SBBL	(BX)(BP*4),AX
	MOVL	AX,(DI)(BP*4)
	INCL	BP
	LOOP	_subloop1

_sub1:
	INCL	DX
	MOVL	DX,CX
	LOOP	_subloop2
	RET

	/* diff[blen:alen-1] = a[blen:alen-1] - 0 */
_subloop2:
	MOVL	(SI)(BP*4),AX
	SBBL	$0,AX
	MOVL	AX,(DI)(BP*4)
	INCL	BP
	LOOP	_subloop2
	RET

