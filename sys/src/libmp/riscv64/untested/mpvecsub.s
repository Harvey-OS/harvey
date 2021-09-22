/*
 *	mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
 *
 *		sum[0:alen] = a[0:alen-1] - b[0:blen-1]
 *
 *	prereq: alen >= blen, sum has room for alen+1 digits
 *
 *		R11 == a	(first arg passed in R11)
 *		R12 == temporary
 *		R13 == carry
 *		R14 == alen
 *		R15 == b
 *		R16 == blen
 *		R17 == sum
 *		R18 == temporary
 *		R19 == temporary
 */
TEXT	mpvecsub(SB),$-4
	MOV	R8, R11			/* first arg */
	MOVW	alen+XLEN(FP), R14
	MOV	b+(2*XLEN)(FP), R15
	MOVW	blen+(3*XLEN)(FP), R16
	MOV	sum+(4*XLEN)(FP), R17

	SUB	R16, R14  /* calculate counter for second loop (alen > blen) */
	MOV	R0, R13

	/* if blen == 0, don't need to subtract it */
	BEQ	R16,_sub1

	/* sum[0:blen-1],carry = a[0:blen-1] - b[0:blen-1] */
_subloop1:
	MOVWU	0(R11), R18
	ADD	$4, R11			/* 4 is sizeof(mpdigit) */
	MOVWU	0(R15), R19
	ADD	$4, R15
	SUBW	R13, R18, R12		/* r12 = *a - carry */
	SLTU	R12, R18, R13
	SUBW	R19, R12, R18		/* r18 = r12 - *b */
	SLTU	R18, R12, R19
	ADDW	R19, R13		/* carry += r19 */
	MOVW	R18, 0(R17)		/* store result digit */
	ADD	$4, R17
	SUB	$1, R16
	BNE	R16, _subloop1

_sub1:
	/* if alen == blen, we're done */
	BEQ	R14, _subend

	/* sum[blen:alen-1],carry = a[blen:alen-1] + 0 + carry */
_subloop2:
	MOVWU	0(R11), R18
	ADD	$4, R11
	SUBW	R13, R18, R12
	SLTU	R12, R18, R13
	MOVW	R12, 0(R17)
	ADD	$4, R17
	SUB	$1, R14
	BNE	R14, _subloop2

	/* sum[alen] = carry */
_subend:
	RET
