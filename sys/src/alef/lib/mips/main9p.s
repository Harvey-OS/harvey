TEXT	_mainp(SB), 1, $16

	MOVW	$setR30(SB), R30
	MOVW	R1, _clock(SB)
	MOVW	inargc-4(FP), R1
	MOVW	$inargv+0(FP), R2
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	JAL	ALEF_init(SB)
loop:
	MOVW	$exits<>(SB), R1
	MOVW	R1, 4(R29)
	JAL	exits(SB)
	MOVW	$_profin(SB), R0	/* force loading of profile */
	JMP	loop

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R1
	RET

DATA	exits<>+0(SB)/4, $"main"
GLOBL	exits<>+0(SB), $5
