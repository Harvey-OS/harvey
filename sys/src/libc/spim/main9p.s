#define NPRIVATES	16

TEXT	_mainp(SB), 1, $(16 + NPRIVATES*4)

	MOVW	$setR30(SB), R30
	MOVW	R1, _clock(SB)

	MOVW	$p-64(SP),R1
	MOVW	R1,_privates+0(SB)
	MOVW	$16,R1
	MOVW	R1,_nprivates+0(SB)

	JAL	_profmain(SB)
	MOVW	__prof+4(SB), R1
	MOVW	R1, __prof+0(SB)
	MOVW	inargc-8(FP), R1
	MOVW	$inargv-4(FP), R2
	ADD	$7, R29
	AND	$~7, R29
	MOVW	R1, 8(R29)
	MOVW	R2, 12(R29)
	JAL	main(SB)
loop:
	MOVW	$exits<>(SB), R1
	MOVW	R1, 8(R29)
	JAL	exits(SB)
	MOVW	$_profin(SB), R0	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
TEXT	_saveret(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R1
	RET

DATA	exits<>+0(SB)/4, $"main"
GLOBL	exits<>+0(SB), $5
