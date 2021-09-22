TEXT getcallerpc(SB), 1, $-4
	MOVQ	-8(RARG), AX
	RET
