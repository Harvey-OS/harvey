#define CMPXCHG(s,d) BYTE $0x0F; BYTE $0xB1; BYTE $((0<<6)|(s<<3)|(d))

TEXT	cas(SB),$0

	MOVL	l+0(FP),BX	/* dst */
	MOVL	i+4(FP),AX	/* accumulator */
	MOVL	i+8(FP),CX	/* src */
	LOCK
	CMPXCHG(1,3)
	JNE	nope
	MOVL	$1, AX
	RET
nope:	MOVL	$0, AX
	RET
