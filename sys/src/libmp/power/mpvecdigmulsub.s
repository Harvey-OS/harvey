#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,
#define	BLT	BC	0xC,0,

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
 *		b = R3
 *		n = R4
 *		m = R5
 *		p = R6
 *		i = R7
 *		hi = R8		- constrained by hardware
 *		lo = R9		- constrained by hardware
 *		oldhi = R10
 *		tmp = R11
 *		borrow = R12
 *		
 */
TEXT	mpvecdigmulsub(SB),$0

	MOVW	n+4(FP),R10
	MOVW	R10,CTR
	MOVW	m+8(FP),R5
	MOVW	p+12(FP),R6
	SUB	$4, R3		/* pre decrement for MOVWU's */
	SUBC	$4, R6		/* pre decrement for MOVWU's and set carry */
	MOVW	XER,R12

	MOVW	R0, R10	

_mulsubloop:
	MOVWU	4(R3),R9	/* lo = b[i] */
	MOVW	4(R6),R11	/* tmp = p[i] */
	MULHWU	R9,R5,R8	/* hi = (b[i] * m)>>32 */
	MULLW	R9,R5,R9	/* lo = b[i] * m */
	ADDC	R10,R9		/* lo += oldhi */
	ADDE	R0,R8		/* hi += carry */
	MOVW	R12,XER
	SUBE	R9,R11		/* tmp -= lo */
	MOVW	XER,R12
	MOVWU	R11,4(R6)	/* p[i] = tmp */
	MOVW	R8,R10		/* oldhi = hi */
	BDNZ	_mulsubloop

	MOVW	4(R6),R11	/* tmp = p[i] */
	MOVW	R12,XER
	SUBE	R10,R11		/* tmp -= lo */
	MOVWU	R11,4(R6)	/* p[i] = tmp */

	/* return -1 if the result was negative, +1 otherwise */
	SUBECC	R0,R0,R3
	BLT	_mulsub2
	MOVW	$1,R3
_mulsub2:
	RETURN
