TEXT getcallerpc(SB), $0
	MOVL	v+0(FP), AX
	MOVL	-4(AX), AX
	RET
