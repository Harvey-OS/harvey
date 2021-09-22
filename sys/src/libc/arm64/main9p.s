#define NPRIVATES	16

TEXT	_mainp(SB), 1, $(2*8 + NPRIVATES*8)
	MOV	$setSB(SB), R28
	MOV	RARG, _tos(SB)

	MOV	$p-(NPRIVATES*8)(SP), R1
	MOV	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	BL	_profmain(SB)
	MOV	_tos(SB), R1
	MOV	8(R1), R0
	MOV	R0, 0(R1)

	MOV	$inargv+0(FP), RARG
	MOV	RARG, 16(RSP)
	MOVW	inargc-8(FP), RARG
	BL	main(SB)
loop:
	MOV	$_exitstr<>(SB), RARG
	BL	exits(SB)
	B	loop

TEXT	_savearg(SB), 1, $0
TEXT	_saveret(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $-4
	MOV	0(RSP), RARG
	RETURN

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
