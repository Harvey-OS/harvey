/*
 *	mpvecdigmulsub(mpdigit *b, int n, mpdigit m, mpdigit *p)
 *
 *	p -= b*m
 *
 *	each step looks like:
 *		hi,lo = m*b[i]
 *		lo += oldhi + carry
 *		hi += carry
 *		p[i] += lo
 *		oldhi = hi
 *
 *	the registers are:
 *		b = R1
 *		n = R4
 *		m = R5
 *		p = R6
 *		i = R7
 *		hi = R8		- constrained by hardware
 *		lo = R9		- constrained by hardware
 *		oldhi = R10
 *		tmp = R11
 *		
 */
TEXT	mpvecdigmulsub(SB),$0

	MOVW	n+4(FP),R4
	MOVW	m+8(FP),R5
	MOVW	p+12(FP),R6

	MOVW	R0, R10		/* oldhi = 0 */
_mulsubloop:
	MOVW	0(R1), R9	/* lo = b[i] */
	ADDU	$4, R1
	MOVW	0(R6), R11	/* tmp = p[i] */
	MULU	R9, R5
	MOVW	HI, R8		/* hi = (b[i] * m)>>32 */
	MOVW	LO, R9		/* lo = b[i] * m */
	ADDU	R10, R9		/* lo += oldhi */
	SGTU	R10, R9, R2
	ADDU	R2, R8		/* hi += carry */
	SUBU	R9, R11, R3	/* tmp -= lo */
	SGTU	R3, R11, R2
	ADDU	R2, R8		/* hi += carry */
	MOVW	R3, 0(R6)	/* p[i] = tmp */
	ADDU	$4, R6
	MOVW	R8, R10		/* oldhi = hi */
	SUBU	$1, R4
	BNE	R4, _mulsubloop

	MOVW	0(R6), R11	/* tmp = p[i] */
	SUBU	R10, R11, R3	/* tmp -= oldhi */
	MOVW	R3, 0(R6)	/* p[i] = tmp */
	SGTU	R3, R11, R1
	BNE	R1, _mulsub2
	MOVW	$1, R1		/* return +1 for positive result */
	RET

_mulsub2:
	MOVW	$-1, R1		/* return -1 for negative result */
	RET
