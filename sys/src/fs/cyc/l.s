#define NOP		MOV	R3,R3

TEXT	_main(SB), $-4

	MOV	$setSB(SB), R28
	SUBO	R3, R3
	MOV	$0x10000, R29

	BAL	main(SB)
	BAL	exits(SB)
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

TEXT	bmovevf(SB), $0		/* bmovevf(vmeaddr, fibrefifo) */
	MOV	va+0(FP),R4
	MOV	fa+4(FP),R5
	MOV	$4, R6

#ifdef	Blockvf
	MOV	$0xA00000D3,R7	/* Vic block mode control */
	MOVOB 	R3,(R7)		/* ROR Burst of 64 accesses */
	NOP;NOP;NOP;NOP
	MOVOB 	$0x20,4(R7)	/* Enable block mode */
	NOP;NOP;NOP;NOP
#endif

vfl:
	SUBO	$1,R6
	CMPI	$0,R6

	MOVQ	0(R4),R8
	MOVQ	R8,(R5)
	MOVQ	16(R4),R8
	MOVQ	R8,(R5)
	MOVQ	32(R4),R8
	MOVQ	R8,(R5)
	MOVQ	48(R4),R8
	MOVQ	R8,(R5)


	ADDO	$64,R4
	BL	vfl

#ifdef	Blockvf
	MOVOB	R3, 4(R7)	/* Disable block mode */
#endif
	RTS

TEXT	bmovefv(SB), $0		/* bmovevf(fibrefifo, vmeaddr) */
	MOV	fa+0(FP),R4
	MOV	va+4(FP),R5
	MOV	$4, R6

#ifdef	Blockfv
	MOV	$0xA00000D3,R7
	MOVOB 	R3, (R7)
	NOP;NOP;NOP;NOP
	MOVOB 	$0x20, 4(R7)
	NOP;NOP;NOP;NOP
#endif

fvl:
	SUBO	$1,R6
	CMPI	$0,R6

	MOVQ	(R4),R8
	MOVQ	R8,0(R5)
	MOVQ	(R4),R8
	MOVQ	R8,16(R5)
	MOVQ	(R4),R8
	MOVQ	R8,32(R5)
	MOVQ	(R4),R8
	MOVQ	R8,48(R5)


	ADDO	$64,R5
	BL	fvl

#ifdef	Blockfv
	MOVOB	R3, 4(R7)
#endif
	RTS
