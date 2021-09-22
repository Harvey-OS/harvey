#define NPRIVATES	16

TEXT	_main(SB), 1, $(2*8 + NPRIVATES*8)
	MOV	$setSB(SB), R28
	MOV	RARG, _tos(SB)

	MOV	$p-(NPRIVATES*8)(SP), R1
	MOV	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	MOV	$inargv+0(FP), RARG
	MOV	RARG, 16(RSP)
	MOVW	inargc-8(FP), RARG
	BL	main(SB)
loop:
	MOV	$_exitstr<>(SB), RARG
	BL	exits(SB)
	B	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
