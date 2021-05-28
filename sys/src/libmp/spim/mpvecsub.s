#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

/*
 *	mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
 *
 *		sum[0:alen] = a[0:alen-1] - b[0:blen-1]
 *
 *	prereq: alen >= blen, sum has room for alen+1 digits
 *
 *		R1 == a	(first arg passed in R1)
 *		R3 == carry
 *		R4 == alen
 *		R5 == b
 *		R6 == blen
 *		R7 == sum
 *		R2 == temporary
 *		R8 == temporary
 *		R9 == temporary
 */
TEXT	mpvecsub(SB),$-4

	MOVW	alen+4(FP), R4
	MOVW	b+8(FP), R5
	MOVW	blen+12(FP), R6
	MOVW	sum+16(FP), R7
	SUBU	R6, R4		/* calculate counter for second loop (alen > blen) */
	MOVW	R0, R3

	/* if blen == 0, don't need to subtract it */
	BEQ	R6,_sub1

	/* sum[0:blen-1],carry = a[0:blen-1] - b[0:blen-1] */
_subloop1:
	MOVW	0(R1), R8
	ADDU	$4, R1
	MOVW	0(R5), R9
	ADDU	$4, R5
	SUBU	R3, R8, R2
	SGTU	R2, R8, R3
	SUBU	R9, R2, R8
	SGTU	R8, R2, R9
	ADDU	R9, R3
	MOVW	R8, 0(R7)
	ADDU	$4, R7
	SUBU	$1, R6
	BNE	R6, _subloop1

_sub1:
	/* if alen == blen, we're done */
	BEQ	R4, _subend

	/* sum[blen:alen-1],carry = a[blen:alen-1] + 0 + carry */
_subloop2:
	MOVW	0(R1), R8
	ADDU	$4, R1
	SUBU	R3, R8, R2
	SGTU	R2, R8, R3
	MOVW	R2, 0(R7)
	ADDU	$4, R7
	SUBU	$1, R4
	BNE	R4, _subloop2

	/* sum[alen] = carry */
_subend:
	RET
