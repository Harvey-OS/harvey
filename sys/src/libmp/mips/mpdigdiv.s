/*
 *  This only works on R[45]000 chips that allow 64 bit
 *  integer arithmetic even when uding 32 bit addresses
 *
 *	R1 = dividend*
 *	R2 = dividend[low]
 *	R3 = dividend[high]
 *	R4 = 32 bit divisor
 *	R5 = quotient*
 */
TEXT	mpdigdiv(SB),$0

	MOVW	0(R1),R2
	MOVW	4(R1),R3
	MOVW	divisor+4(FP),R4
	MOVW	quotient+8(FP),R5

	/* divisor == 0 */
	BEQ	R4,_digovfl

	/* dividend >= 2^32 * divisor */
	SGTU	R4,R3,R7
	BEQ	R7,_digovfl

_digdiv1:
	SLLV	$32,R2
	SLLV	$32,R3
	SRLV	$32,R2
	ADDVU	R2,R3
	SLLV	$32,R4
	SRLV	$32,R4
	DIVVU	R4,R3
	MOVW	LO,R1
	MOVW	R1,0(R5)
	RET

_digovfl:
	MOVW	$-1,R1
	MOVW	R1,0(R5)
	RET

