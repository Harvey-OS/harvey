#define STACK	(0x5003fff0)
#define	PCW	12
#define	mypcw	0x6215
#define NOOP	BRA	FALSE, (R0)

TEXT	_main(SB), 1, $-4
	NOOP
	MOVW	$mypcw, R2
	MOVH	R2, C(PCW)
	NOOP
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
	NOOP
	NOOP
	NOOP

	MOVW	$edata(SB), R5
cloop:	DO	R6,clear
	MOVW	R0,(R5)+
clear:	DOEND	cloop

nocopy:
	MOVW	$main(SB), R4
	MOVW	$STACK, R1
	CALL	(R4)
loop:
	JMP	loop
