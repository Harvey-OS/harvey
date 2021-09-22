	TEXT	b+0(SB),0,$8
	MOV	p1+16(FP),R9
	MOV	R0,a1+0(FP)
	MOV	R9,R0
	BL	,atoll+0(SB)
	MOV	a1+0(FP),R10
	MOVW	i1+8(FP),R9
	ADD	R9,R10,R4
	ADD	R0,R4,R0
	RETURN	,

	TEXT	a+0(SB),0,$24
	MOVW	i1+8(FP),R11
	MOV	p1+16(FP),R10
	MOVW	R11,16(R31)
	MOV	R10,24(R31)
	BL	,b+0(SB)
	RETURN	,

	TEXT	main+0(SB),0,$32
	DATA	.string<>+0(SB)/8,$"stuff %l"
	MOV	$34,R0
	MOVW	$23,R4
	MOVW	R4,16(R31)
	MOV	$0,24(R31)
	BL	,a+0(SB)
	MOV	R0,.safe-8(SP)
	MOV	$.string<>+0(SB),R0
	MOV	.safe-8(SP),R2
	MOV	R2,16(R31)
	BL	,print+0(SB)
	MOV	$0,R0
	BL	,exits+0(SB)
	RETURN	,
	DATA	.string<>+8(SB)/8,$"d\n\z\z\z\z\z\z"
	GLOBL	.string<>+0(SB),$16
