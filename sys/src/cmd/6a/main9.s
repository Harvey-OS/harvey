	TEXT	_main(SB), $-4

	MOV	$setSB(SB), R28
	SUBO	R3, R3
	MOV	$0x10000, R29

	BAL	main(SB)
	BAL	exits(SB)
	B	0(PC)

	TEXT	exits(SB), $0
	LONG	$0x66003e00
	LONG	$0x66003f80
	LONG	$0xfeedface
	B	0(PC)

	TEXT	sysctl(SB),$0
	MOV	n+0(FP),R4
	MOV	n+4(FP),R5
	MOV	n+8(FP),R6
	SYSCTL	R4,R5,R6
	RTS

	TEXT	modpc(SB), $0
	MOV	n+0(FP),R5
	MOV	n+4(FP),R4
	MODPC	R5, R5, R4
	RTS
