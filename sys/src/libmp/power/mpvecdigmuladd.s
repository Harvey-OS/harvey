#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

/*
 *	mpvecdigmuladd(mpdigit *b, int n, mpdigit m, mpdigit *p)
 *
 *	p += b*m
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
 *		
 */
TEXT	mpvecdigmuladd(SB),$0

	MOVW	n+4(FP),R4
	MOVW	m+8(FP),R5
	MOVW	p+12(FP),R6
	SUB	$4, R3		/* pre decrement for MOVWU's */
	SUB	$4, R6		/* pre decrement for MOVWU's */

	MOVW	R0, R10	
	MOVW	R0, XER
	MOVW	R4, CTR
_muladdloop:
	MOVWU	4(R3),R9	/* lo = b[i] */
	MOVW	4(R6),R11	/* tmp = p[i] */
	MULHWU	R9,R5,R8	/* hi = (b[i] * m)>>32 */
	MULLW	R9,R5,R9	/* lo = b[i] * m */
	ADDC	R10,R9		/* lo += oldhi */
	ADDE	R0,R8		/* hi += carry */
	ADDC	R9,R11		/* tmp += lo */
	ADDE	R0,R8		/* hi += carry */
	MOVWU	R11,4(R6)	/* p[i] = tmp */
	MOVW	R8,R10		/* oldhi = hi */
	BDNZ	_muladdloop

	MOVW	4(R6),R11	/* tmp = p[i] */
	ADDC	R10,R11
	MOVWU	R11,4(R6)	/* p[i] = tmp */

	RETURN
