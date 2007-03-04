TEXT	_mulv(SB), $0
	MOVW	4(FP),R9	/* l0 */
	MOVW	8(FP),R10	/* h0 */
	MOVW	12(FP),R4	/* l1 */
	MOVW	16(FP),R5	/* h1 */
	MULLU	R9,R4,(R6, R7)	/* l0*l1 */
	MUL	R9,R5,R5	/* l0*h1 */
	MUL	R10,R4,R4	/* h0*l1 */
	ADD	R4,R6
	ADD	R5,R6
	MOVW	R6,4(R0)
	MOVW	R7,0(R0)
	RET
