#define NPRIVATES	16

TEXT	_main(SB), 1, $(4*XLEN + NPRIVATES*XLEN)

	MOV	$setSB(SB), R3
	MOV	R8, _tos(SB)

	MOV	$p-(NPRIVATES*XLEN)(SP), R9
	MOV	R9, _privates(SB)
	MOV	$NPRIVATES, R9
	MOV	R9, _nprivates(SB)

	MOV	inargc-XLEN(FP), R8
	MOV	$inargv+0(FP), R10
	MOV	R8, XLEN(R2)
	MOV	R10, (2*XLEN)(R2)
	JAL	R1, main(SB)
loop:
	MOV	$_exitstr<>(SB), R8
	MOV	R8, XLEN(R2)
	JAL	R1, exits(SB)
	JMP	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
