#undef	XLEN
#define	XLEN 4

TEXT	_mulv(SB), $0			/* TODO: revisit; needed on rv64? */
	MOVW	XLEN(FP), R9		// x.lo
	MOVW	(2*XLEN)(FP), R10	// x.hi
	MOVW	(3*XLEN)(FP), R11	// y.lo
	MOVW	(4*XLEN)(FP), R12	// y.hi
	MULHU	R11, R9, R14		// (x.lo*y.lo).hi
	MUL	R11, R9, R13		// (x.lo*y.lo).lo
	MUL	R10, R11, R15		// (x.hi*y.lo).lo
	ADD	R15, R14
	MUL	R9, R12, R15		// (x.lo*y.hi).lo
	ADD	R15, R14
	MOVW	R13, 0(R8)
	MOVW	R14, XLEN(R8)
	RET
