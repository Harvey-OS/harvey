#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

/*
 *	mpvecsub(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *diff)
 *
 *		diff[0:alen-1] = a[0:alen-1] - b[0:blen-1]
 *
 *	prereq: alen >= blen, diff has room for alen digits
 *
 *		R3 == a
 *		R4 == alen
 *		R5 == b
 *		R6 == blen
 *		R7 == diff
 *		R8 == temporary
 *		R9 == temporary
 */
TEXT	mpvecsub(SB),$-4

	MOVW	alen+4(FP),R4
	MOVW	b+8(FP),R5
	MOVW	blen+12(FP),R6
	MOVW	diff+16(FP),R7
	SUB	R6, R4		/* calculate counter for second loop (alen > blen) */
	SUB	$4, R3		/* pre decrement for MOVWU's */
	SUB	$4, R5		/* pre decrement for MOVWU's */
	SUBC	$4, R7		/* pre decrement for MOVWU's and set carry */

	/* skip subraction if b is zero */
	CMP	R0,R6
	BEQ	_sub1

	/* diff[0:blen-1],borrow = a[0:blen-1] - b[0:blen-1] */
	MOVW	R6, CTR
_subloop1:
	MOVWU	4(R3), R8
	MOVWU	4(R5), R9
	SUBE	R9, R8, R8
	MOVWU	R8, 4(R7)
	BDNZ	_subloop1

_sub1:
	/* skip subtraction if a is zero */
	CMP	R0, R4
	BEQ	_subend

	/* diff[blen:alen-1] = a[blen:alen-1] - 0 + carry */
	MOVW	R4, CTR
_subloop2:
	MOVWU	4(R3), R8
	SUBE	R0, R8
	MOVWU	R8, 4(R7)
	BDNZ	_subloop2
_subend:
	RETURN

