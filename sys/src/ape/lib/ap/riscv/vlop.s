TEXT	_mulv(SB), $0
	MOVW	4(FP), R9	// x.lo
	MOVW	8(FP), R10	// x.hi
	MOVW	12(FP), R11	// y.lo
	MOVW	16(FP), R12	// y.hi
	MULHU	R11, R9, R14	// (x.lo*y.lo).hi
	MUL	R11, R9, R13	// (x.lo*y.lo).lo
	MUL	R10, R11, R15	// (x.hi*y.lo).lo
	ADD	R15, R14
	MUL	R9, R12, R15	// (x.lo*y.hi).lo
	ADD	R15, R14
	MOVW	R13, 0(R8)
	MOVW	R14, 4(R8)
	RET
