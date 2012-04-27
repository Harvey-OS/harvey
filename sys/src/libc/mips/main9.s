#define NPRIVATES	16

TEXT	_main(SB), 1, $(16 + NPRIVATES*4)

	MOVW	$setR30(SB), R30
	MOVW	R1, _tos(SB)

	MOVW	$p-64(SP), R1
	MOVW	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	MOVW	inargc-4(FP), R1
	MOVW	$inargv+0(FP), R2
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	JAL	main(SB)
loop:
	MOVW	$_exitstr<>(SB), R1
	MOVW	R1, 4(R29)
	JAL	exits(SB)
	JMP	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
