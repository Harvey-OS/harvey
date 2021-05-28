	TEXT	memccpy(SB),$0

	MOVL	n+24(FP), CX
	CMPL	CX, $0
	JEQ	none
	MOVQ	p2+8(FP), DI
	MOVBLZX	c+16(FP), AX
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
	MOVL	n+24(FP), BX
	JMP	memcpy

/*
 * if found, set count to bytes thru character
 */
found:
	MOVQ	DI, AX
	SUBQ	p2+8(FP), AX
	MOVQ	AX, BX
	ADDQ	RARG, AX

/*
 * copy the memory
 */

memcpy:
	MOVQ	RARG, DI
	MOVQ	p2+8(FP), SI
/*
 * copy whole longs, if aligned
 */
	MOVQ	DI, DX
	ORQ	SI, DX
	ANDL	$3, DX
	JNE	c3
	MOVL	BX, CX
	SHRQ	$2, CX
	REP;	MOVSL
/*
 * copy the rest, by bytes
 */
	ANDL	$3, BX
c3:
	MOVL	BX, CX
	REP;	MOVSB

	RET
