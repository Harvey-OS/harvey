#define NPRIVATES	16

TEXT	_main(SB), 1, $(16 + NPRIVATES*4)

	MOVW	$setSB(SB), R2
	MOVW	R3, _tos(SB)

	MOVW	$p-64(SP), R4
	MOVW	R4, _privates+0(SB)
	MOVW	$16, R4
	MOVW	R4, _nprivates+0(SB)

	MOVW	inargc-4(FP), R3
	MOVW	$inargv+0(FP), R4
	MOVW	R3, 4(R1)
	MOVW	R4, 8(R1)
	BL	main(SB)
loop:
	MOVW	$_exitstr<>(SB), R3
	MOVW	R3, 4(R1)
	BL	exits(SB)
	BR	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
