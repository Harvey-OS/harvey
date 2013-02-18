TEXT memmove(SB), $0
	MOVL	p1+0(FP), DI
	MOVL	DI, AX			/* return value */
	MOVL	p2+4(FP), SI
	MOVL	n+8(FP), BX
	CMPL	BX, $0
	JGT	_ok
	JEQ	_return			/* nothing to do if n == 0 */
	MOVL	$0, SI			/* fault if n < 0 */

/*
 * check and set for backwards:
 *	(p2 < p1) && ((p2+n) > p1)
 */
_ok:
	CMPL	SI, DI
	JGT	_forward
	JEQ	_return			/* nothing to do if p2 == p1 */
	MOVL	SI, DX
	ADDL	BX, DX
	CMPL	DX, DI
	JGT	_back

/*
 * copy whole longs
 */
_forward:
	MOVL	BX, CX
	CLD
	SHRL	$2, CX
	ANDL	$3, BX
	REP;	MOVSL

/*
 * copy the rest, by bytes
 */
	JEQ	_return			/* flags set by above ANDL */
	MOVL	BX, CX
	REP;	MOVSB

	RET

/*
 * whole thing backwards has
 * adjusted addresses
 */
_back:
	ADDL	BX, DI
	ADDL	BX, SI
	STD
	SUBL	$4, DI
	SUBL	$4, SI
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
	JEQ	_return			/* flags set by above ANDL */

	ADDL	$3, DI
	ADDL	$3, SI
	MOVL	BX, CX
	REP;	MOVSB

_return:
	RET
