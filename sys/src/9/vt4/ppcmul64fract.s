
TEXT mul64fract(SB), 1, $0
	MOVW	a0+8(FP), R9
	MOVW	a1+4(FP), R10
	MOVW	b0+16(FP), R4
	MOVW	b1+12(FP), R5

	MULLW	R10, R5, R13		/* c2 = lo(a1*b1) */

	MULLW	R10, R4, R12		/* c1 = lo(a1*b0) */
	MULHWU	R10, R4, R7		/* hi(a1*b0) */
	ADD	R7, R13			/* c2 += hi(a1*b0) */

	MULLW	R9, R5, R6		/* lo(a0*b1) */
	MULHWU	R9, R5, R7		/* hi(a0*b1) */
	ADDC	R6, R12			/* c1 += lo(a0*b1) */
	ADDE	R7, R13			/* c2 += hi(a0*b1) + carry */

	MULHWU	R9, R4, R7		/* hi(a0*b0) */
	ADDC	R7, R12			/* c1 += hi(a0*b0) */
	ADDE	R0, R13			/* c2 += carry */

	MOVW	R12, 4(R3)
	MOVW	R13, 0(R3)
	RETURN
