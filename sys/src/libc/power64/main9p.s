#define NPRIVATES	16

TEXT	_mainp(SB), 1, $(2*8 + NPRIVATES*8)

	MOVD	$setSB(SB), R2
	MOVD	R3, _tos(SB)

	MOVD	$p-64(SP), R4
	MOVD	R4, _privates+0(SB)
	MOVW	$16, R4
	MOVW	R4, _nprivates+0(SB)

	BL		_profmain(SB)
	MOVD	_tos(SB), R3
	MOVD	8(R3), R4
	MOVD	R4, 0(R3)
	MOVW	inargc-4(FP), R3
	MOVD	$inargv+0(FP), R4
	MOVD	R4, 16(R1)
	BL		main(SB)
loop:
	MOVD	$exits<>(SB), R3
	BL	exits(SB)
	MOVD	$_profin(SB), R3	/* force loading of profile */
	BR	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	MOVD	argp-8(FP), R3
	RETURN

DATA	exits<>+0(SB)/4, $"main"
GLOBL	exits<>+0(SB), $5
