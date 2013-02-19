TEXT memcpy(SB), $0
	MOVQ	RARG, DI
	MOVQ	DI, AX			/* return value */
	MOVQ	p2+8(FP), SI
	MOVL	n+16(FP), BX
	CMPL	BX, $0
	JGT	_ok
	JEQ	_return			/* nothing to do if n == 0 */
	MOVL	$0, SI			/* fault if n < 0 */

/*
 * check and set for backwards:
 *	(p2 < p1) && ((p2+n) > p1)
 */
_ok:
	CMPQ	SI, DI
	JGT	_forward
	JEQ	_return			/* nothing to do if p2 == p1 */
	MOVQ	SI, DX
	ADDQ	BX, DX
	CMPQ	DX, DI
	JGT	_back

/*
 * copy whole longs if aligned
 */
_forward:
	CLD
	MOVQ	SI, DX
	ORQ	DI, DX
	ANDL	$3, DX
	JNE	c3f
	MOVQ	BX, CX
	SHRQ	$2, CX
	ANDL	$3, BX
	REP;	MOVSL

/*
 * copy the rest, by bytes
 */
	JEQ	_return			/* flags set by above ANDL */
c3f:
	MOVL	BX, CX
	REP;	MOVSB

	RET

/*
 * whole thing backwards has
 * adjusted addresses
 */
_back:
	ADDQ	BX, DI
	ADDQ	BX, SI
	STD
	SUBQ	$4, DI
	SUBQ	$4, SI
/*
 * copy whole longs, if aligned
 */
	MOVQ	DI, DX
	ORQ	SI, DX
	ANDL	$3, DX
	JNE	c3b
	MOVL	BX, CX
	SHRQ	$2, CX
	ANDL	$3, BX
	REP;	MOVSL
/*
 * copy the rest, by bytes
 */
	JEQ	_return			/* flags set by above ANDL */

c3b:
	ADDQ	$3, DI
	ADDQ	$3, SI
	MOVL	BX, CX
	REP;	MOVSB

_return:
	RET
