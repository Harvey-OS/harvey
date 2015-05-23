TEXT getcallerpc(SB), $0
	MOVQ	-8(RARG), AX
	RET
