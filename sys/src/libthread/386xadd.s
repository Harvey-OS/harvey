/*#define XADDL(s,d) BYTE $0x0F; BYTE $0xC1; BYTE $((0<<6)|(s<<3)|(d))*/

/*TEXT	xadd(SB),$0	/* long xadd(long *, long); */

/*	MOVL	l+0(FP),BX
/*	MOVL	i+4(FP),AX
/*	LOCK
/*	XADDL(0,3)
/*	RET
*/

TEXT	_xinc(SB),$0	/* void _xinc(long *); */

	MOVL	l+0(FP),AX
	LOCK
	INCL	0(AX)
	RET

TEXT	_xdec(SB),$0	/* long _xdec(long *); */

	MOVL	l+0(FP),AX
	LOCK
	DECL	0(AX)
	JZ	iszero
	MOVL	$1, AX
	RET
iszero:
	MOVL	$0, AX
	RET
