#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

/*
 *	mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
 *
 *		sum[0:alen] = a[0:alen-1] + b[0:blen-1]
 *
 *	prereq: alen >= blen, sum has room for alen+1 digits
 *
 *		R3 == a	(first arg passed in R3)
 *		R4 == alen
 *		R5 == b
 *		R6 == blen
 *		R7 == sum
 *		R8 == temporary
 *		R9 == temporary
 */
TEXT	mpvecadd(SB),$-4

	MOVW	alen+4(FP), R4
	MOVW	b+8(FP), R5
	MOVW	blen+12(FP), R6
	MOVW	sum+16(FP), R7
	SUB	R6, R4		/* calculate counter for second loop (alen > blen) */
	SUB	$4, R3		/* pre decrement for MOVWU's */
	SUB	$4, R5		/* pre decrement for MOVWU's */
	SUB	$4, R7		/* pre decrement for MOVWU's */
	MOVW	R0, XER		/* zero carry going in */

	/* if blen == 0, don't need to add it in */
	CMP	R0, R6
	BEQ	_add1

	/* sum[0:blen-1],carry = a[0:blen-1] + b[0:blen-1] */
	MOVW	R6, CTR
_addloop1:
	MOVWU	4(R3), R8
	MOVWU	4(R5), R9
	ADDE	R8, R9
	MOVWU	R9, 4(R7)
	BDNZ	_addloop1

_add1:
	/* if alen == blen, we're done */
	CMP	R0, R4
	BEQ	_addend

	/* sum[blen:alen-1],carry = a[blen:alen-1] + 0 + carry */
	MOVW	R4, CTR
_addloop2:
	MOVWU	4(R3), R8
	ADDE	R0, R8
	MOVWU	R8, 4(R7)
	BDNZ	_addloop2

	/* sum[alen] = carry */
_addend:
	ADDE	R0, R0, R8
	MOVW	R8, 4(R7)
	RETURN
