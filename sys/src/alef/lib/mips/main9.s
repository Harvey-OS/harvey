	TEXT	_main(SB), 1, $16

	MOVW	$setR30(SB), R30
	MOVW	R1, _clock(SB)

	MOVW	inargc-4(FP), R1
	MOVW	$inargv+0(FP), R2
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	JAL	ALEF_init(SB)
loop:
	JMP	loop

GLOBL	_clock(SB), $4
