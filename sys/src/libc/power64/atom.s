TEXT ainc(SB), 1, $-4				/* long ainc(long *); */
	BR	_trap
	RET

TEXT adec(SB), 1, $-4				/* long adec(long*); */
	BR	_trap
	RET

TEXT _xinc(SB), 1, $-4				/* void _xinc(long *); */
	BR	_trap
	RET

TEXT _xdec(SB), 1, $-4				/* long _xdec(long *); */
	BR	_trap
	RET

/*
 * int cas(uint* p, int ov, int nv);
 */
TEXT cas(SB), 1, $-4
	BR	_trap
	RET

/*
 * int casv(u64int* p, u64int ov, u64int nv);
 */
TEXT casv(SB), 1, $-4
	BR	_trap
	RET

_trap:
	MOVD	$0, R0
	MOVD	0(R0), R0
	RET
