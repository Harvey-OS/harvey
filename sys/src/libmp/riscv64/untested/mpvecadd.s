/*
 *	mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
 *
 *		sum[0:alen] = a[0:alen-1] + b[0:blen-1]
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
TEXT	mpvecadd(SB),$-4
	MOV	R8, R11			/* first arg */
	MOVW	alen+XLEN(FP), R14
	MOV	b+(2*XLEN)(FP), R15
	MOVW	blen+(3*XLEN)(FP), R16
	MOV	sum+(4*XLEN)(FP), R17

	SUB	R16, R14  /* calculate counter for second loop (alen > blen) */
	MOV	R0, R13

	/* if blen == 0, don't need to add it in */
	BEQ	R16,_add1

	/* sum[0:blen-1],carry = a[0:blen-1] + b[0:blen-1] */
_addloop1:
	MOVWU	0(R11), R18
	ADD	$4, R11
	MOVWU	0(R15), R19
	ADD	$4, R15
// mips SGTU b, a, dest == riscv SLTU b, a, dest
	ADDW	R13, R18		// r18 += carry
	SLTU	R13, R18, R13
	ADDW	R18, R19
	SLTU	R18, R19, R12
	ADDW	R12, R13
	MOVW	R19, 0(R17)
	ADD	$4, R17
	SUB	$1, R16
	BNE	R16, _addloop1

_add1:
	/* if alen == blen, we're done */
	BEQ	R14, _addend

	/* sum[blen:alen-1],carry = a[blen:alen-1] + 0 + carry */
_addloop2:
	MOVWU	0(R11), R18
	ADD	$4, R11
	ADDW	R13, R18
	SLTU	R13, R18, R13
	MOVW	R18, 0(R17)
	ADD	$4, R17
	SUB	$1, R14
	BNE	R14, _addloop2

	/* sum[alen] = carry */
_addend:
	MOVW	R13, 0(R17)
	RET
