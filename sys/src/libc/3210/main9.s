TEXT	_main(SB), 1, $16

	/*
	 * move stuff to internal ram
	 */
	MOVW	$0x5003e000, R4
	MOVW	$_ramsize(SB), R6
	CMP	R6,R0
	BRA	EQ,nocopy

	MOVW	$edata(SB), R5
	SRL	$2, R6
	SUB	$1, R6
	BMOVW	R6, (R5)+, (R4)+

	MOVW	$edata(SB), R5
cloop:	DO	R6,clear
	MOVW	R0,(R5)+
clear:	DOEND	cloop

nocopy:
	MOVW	$inargc+0(FP), R3
	MOVW	(R3), R3
	MOVW	$inargv+4(FP), R4
	ADD	$8, R1, R5
	MOVW	R4, (R5)
	CALL	main(SB)
loop:
	MOVW	$exitstr<>(SB), R3
	CALL	exits(SB)
	JMP	loop

DATA	exitstr<>+0(SB)/4, $"main"
GLOBL	exitstr<>+0(SB), $5
