	TEXT	memcpy(SB), $0

	MOVL	p1+0(FP), DI
	MOVL	p2+4(FP), SI
	MOVL	n+8(FP), BX
	CMPL	BX, $0
	JGE	ok
	MOVL	$0, SI
ok:
	CLD
/*
 * check and set for backwards
 */
	CMPL	SI, DI
	JLS	back
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

	MOVL	p+0(FP),AX
	RET
/*
 * whole thing backwards has
 * adjusted addresses
 */
back:
	ADDL	BX, DI
	ADDL	BX, SI
	SUBL	$4, DI
	SUBL	$4, SI
	STD
/*
 * copy whole longs
 */
	MOVL	BX, CX
	SHRL	$2, CX
	ANDL	$3, BX
	REP;	MOVSL
/*
 * copy the rest, by bytes
 */
	ADDL	$3, DI
	ADDL	$3, SI
	MOVL	BX, CX
	REP;	MOVSB

	MOVL	p+0(FP),AX
	RET
