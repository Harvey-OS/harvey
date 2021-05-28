TEXT	_mulv(SB), $0
	MOVW	8(FP), R2	/* hi1 */
	MOVW	4(FP), R3	/* lo1 */
	MOVW	16(FP), R4	/* hi2 */
	MOVW	12(FP), R5	/* lo2 */
	MULU	R5, R3	/* lo1*lo2 -> hi:lo*/
	MOVW	LO, R6
	MOVW	HI, R7
	MULU	R3, R4	/* lo1*hi2 -> _:hi */
	MOVW	LO, R8
	ADDU	R8, R7
	MULU	R2, R5	/* hi1*lo2 -> _:hi */
	MOVW	LO, R8
	ADDU	R8, R7
	MOVW	R6, 0(R1)	/* lo */
	MOVW	R7, 4(R1)	/* hi */
	RET
