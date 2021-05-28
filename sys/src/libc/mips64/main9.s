#define NPRIVATES	16

TEXT	_main(SB), 1, $(16 + NPRIVATES*8)

	MOVV	$setR30(SB), R30
	MOVV	R1, _tos(SB)

	MOVV	$p-(NPRIVATES*8)(SP), R1
	MOVV	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	MOVW	inargc-4(FP), R1
	MOVV	$inargv+0(FP), R2
	MOVW	R1, 12(R29)
	MOVV	R2, 16(R29)
	JAL	main(SB)
loop:
	MOVV	$_exitstr<>(SB), R1
	MOVV	R1, 8(R29)
	JAL	exits(SB)
	JMP	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
