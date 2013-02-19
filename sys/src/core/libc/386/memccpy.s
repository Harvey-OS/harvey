	TEXT	memccpy(SB),$0

	MOVL	n+12(FP), CX
	CMPL	CX, $0
	JEQ	none
	MOVL	p2+4(FP), DI
	MOVBLZX	c+8(FP), AX
	CLD
/*
 * find the character in the second string
 */

	REPN;	SCASB
	JEQ	found

/*
 * if not found, set count to 'n'
 */
none:
	MOVL	$0, AX
	MOVL	n+12(FP), BX
	JMP	memcpy

/*
 * if found, set count to bytes thru character
 */
found:
	MOVL	DI, AX
	SUBL	p2+4(FP), AX
	MOVL	AX, BX
	ADDL	p1+0(FP), AX

/*
 * copy the memory
 */

memcpy:
	MOVL	p1+0(FP), DI
	MOVL	p2+4(FP), SI
/*
 * copy whole longs
 */
	MOVL	BX, CX
	SHRL	$2, CX
	REP;	MOVSL
/*
 * copy the rest, by bytes
 */
	ANDL	$3, BX
	MOVL	BX, CX
	REP;	MOVSB

	RET
