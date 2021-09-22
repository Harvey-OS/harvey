#define RARG	R8

TEXT	getcallerpc(SB), $0
	MOVW	0(SP), RARG
	RET

