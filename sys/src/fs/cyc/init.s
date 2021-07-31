/*
 * Initial peice of assembler to get the dram going
 * Must run at 0xF0C00048
 */
	TEXT	initip(SB), $-4		/* Initial entry point */

	MOV	$setSB(SB), R28
	MOV	$0x400, R29		/* Put stack in fast data ram */

	MOV	$0, R4
	MOV	$0, R5
	MOV	$0xa00000b3, R3
	MOV	$0x70,R4		/* Set refresh enable */
	MOVOB	R4, (R3)		/* Write and verify */
	MOVOB	(R3), R5
	CMPO	R4, R5
	BE	sip2

	MOV	$0x00a00000,R6

wait:	SUBO	$1, R6
	CMPO	$0, R6
	BNE	wait
	B	initip(SB)		/* and try again */

sip2:	/* VIC out of reset state - disable refresh and cont */
	MOV	$0x60,R4
	MOVOB	R4, (R3)

	/* Set up VIC chip to allow access to dram */
	MOV	$0xA0000000, R4
	MOVOB	$0x49, 0xa7(R4)
	MOVOB	$0x40, 0xaf(R4)
	MOVOB	$0x70, 0xb3(R4)
	MOVOB	$0x12, 0xc3(R4)
	MOVOB	$0x30, 0xc7(R4)
	MOVOB	$0x16, 0xcb(R4)
	MOVOB	$0x30, 0xcf(R4)

	SUBO	R3,R3
	BAL	main(SB)

l1:	B	l1
