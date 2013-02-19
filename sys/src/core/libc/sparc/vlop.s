TEXT	_mulv(SB), $0
	MOVW	u1+8(FP), R8
	MOVW	u2+16(FP), R13

	MOVW	R13, R16		/* save low parts for later */
	MOVW	R8, R12

	/*
	 * unsigned 32x32 => 64 multiply
	 */
	CMP	R13, R8
	BLE	mul1
	MOVW	R12, R13
	MOVW	R16, R8
mul1:
	MOVW	R13, Y
	ANDNCC	$0xFFF, R13, R0
	BE	mul_shortway
	ANDCC	R0, R0, R9		/* zero partial product and clear N and V cond's */

	/* long multiply */
	MULSCC	R8, R9, R9		/* 0 */
	MULSCC	R8, R9, R9		/* 1 */
	MULSCC	R8, R9, R9		/* 2 */
	MULSCC	R8, R9, R9		/* 3 */
	MULSCC	R8, R9, R9		/* 4 */
	MULSCC	R8, R9, R9		/* 5 */
	MULSCC	R8, R9, R9		/* 6 */
	MULSCC	R8, R9, R9		/* 7 */
	MULSCC	R8, R9, R9		/* 8 */
	MULSCC	R8, R9, R9		/* 9 */
	MULSCC	R8, R9, R9		/* 10 */
	MULSCC	R8, R9, R9		/* 11 */
	MULSCC	R8, R9, R9		/* 12 */
	MULSCC	R8, R9, R9		/* 13 */
	MULSCC	R8, R9, R9		/* 14 */
	MULSCC	R8, R9, R9		/* 15 */
	MULSCC	R8, R9, R9		/* 16 */
	MULSCC	R8, R9, R9		/* 17 */
	MULSCC	R8, R9, R9		/* 18 */
	MULSCC	R8, R9, R9		/* 19 */
	MULSCC	R8, R9, R9		/* 20 */
	MULSCC	R8, R9, R9		/* 21 */
	MULSCC	R8, R9, R9		/* 22 */
	MULSCC	R8, R9, R9		/* 23 */
	MULSCC	R8, R9, R9		/* 24 */
	MULSCC	R8, R9, R9		/* 25 */
	MULSCC	R8, R9, R9		/* 26 */
	MULSCC	R8, R9, R9		/* 27 */
	MULSCC	R8, R9, R9		/* 28 */
	MULSCC	R8, R9, R9		/* 29 */
	MULSCC	R8, R9, R9		/* 30 */
	MULSCC	R8, R9, R9		/* 31 */
	MULSCC	R0, R9, R9		/* 32; shift only; r9 is high part */

	/*
	 * need to correct top word if top bit set
	 */
	CMP	R8, R0
	BGE	mul_tstlow
	ADD	R13, R9			/* adjust the high parts */

mul_tstlow:
	MOVW	Y, R13			/* get low part */
	BA	mul_done

mul_shortway:
	ANDCC	R0, R0, R9		/* zero partial product and clear N and V cond's */
	MULSCC	R8, R9, R9		/*  0 */
	MULSCC	R8, R9, R9		/*  1 */
	MULSCC	R8, R9, R9		/*  2 */
	MULSCC	R8, R9, R9		/*  3 */
	MULSCC	R8, R9, R9		/*  4 */
	MULSCC	R8, R9, R9		/*  5 */
	MULSCC	R8, R9, R9		/*  6 */
	MULSCC	R8, R9, R9		/*  7 */
	MULSCC	R8, R9, R9		/*  8 */
	MULSCC	R8, R9, R9		/*  9 */
	MULSCC	R8, R9, R9		/* 10 */
	MULSCC	R8, R9, R9		/* 11 */
	MULSCC	R0, R9, R9		/* 12; shift only; r9 is high part */

	MOVW	Y, R8			/* make low part of partial low part & high part */
	SLL	$12, R9, R13
	SRL	$20, R8
	OR	R8, R13

	SRA	$20, R9			/* high part */

mul_done:

	/*
	 * mul by high halves if needed
	 */
	MOVW	R13, 4(R7)
	MOVW	u2+12(FP), R11
	CMP	R11, R0
	BE	nomul1
	MUL	R11, R12
	ADD	R12, R9

nomul1:
	MOVW	u1+4(FP), R11
	CMP	R11, R0
	BE	nomul2
	MUL	R11, R16
	ADD	R16, R9

nomul2:

	MOVW	R9, 0(R7)
	RETURN	
