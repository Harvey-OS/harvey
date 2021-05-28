#define NPRIVATES	16

TEXT	_main(SB), 1, $(2*8 + NPRIVATES*8)

	MOVD	$setSB(SB), R2
	MOVD	R3, _tos(SB)

	MOVD	$p-64(SP), R4
	MOVD	R4, _privates+0(SB)
	MOVW	$16, R4
	MOVW	R4, _nprivates+0(SB)

	MOVW	inargc-8(FP), R3
	MOVD	$inargv+0(FP), R4
	MOVW	R4, 16(R1)
	BL	main(SB)
loop:
	MOVD	$_exitstr<>(SB), R3
	BL	exits(SB)
	BR	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
