/*TEXT	xadd(SB),$0	/* long xadd(long *, long); */

/*	MOVL	i+8(FP),AX
/*	LOCK
/*	XADDL	AX, (RARG)
/*	RET
*/

TEXT	_xinc(SB),$0	/* void _xinc(long *); */

	LOCK; INCL	0(RARG)
	RET

TEXT	_xdec(SB),$0	/* long _xdec(long *); */

	MOVL	$0, AX
	MOVL	$1, BX
	LOCK; DECL	0(RARG)
	CMOVLNE	BX, AX
	RET
