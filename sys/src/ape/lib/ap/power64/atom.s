/* stubs */
TEXT ainc(SB), 1, $-4				/* long ainc(long *); */
TEXT adec(SB), 1, $-4				/* long adec(long*); */
/*
 * int cas(uint* p, int ov, int nv);
 */
TEXT cas(SB), 1, $-4
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
