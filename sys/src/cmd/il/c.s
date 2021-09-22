/*
	ia c.s && 8.out -la c.i
	il -la c.i
	win /sys/src/cmd/db/8.out i.out
*/
TEXT	_main(SB), 0, $-4
	MOVW	$0x1001, R9
	MOVW	$0x1004(SP), R10
	MOVW	R12, 0x1004(SP) 
	ADD		$0x1002, R11
	MOVB	R12, bumph(SB)
	MOVH	R13, gumph(SB)
	MOVW	humph(SB), R14
	MOVHU	gumph(SB), R15
	RET
TEXT	_next(SB), 0, $-4
	RET
	GLOBL	space(SB),$4
	GLOBL	humph(SB),$4096
	GLOBL	bumph(SB),$4096
	GLOBL	gumph(SB),$4096
