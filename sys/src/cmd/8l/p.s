/* Zpseudo */
	TEXT	proto(SB), $4
/* Zbr */
	JLE	out
/* Zcall */
	CALL	proto(SB)
/* Zib_ */
	ADDB	$5, AL
/* Zib_rp */
	MOVB	$5, AL
/* Zibo_m */
	ADDB	$5, BL
/* Zil_ */
	ADDL	$0x5000, AX
/* Zil_rp */
	MOVL	$0x5000, AX
/* Zilo_m */
	MOVL	$0x5000, x(SB)
/* Zm_o */
	PUSHL	x(SB)
/* Zm_r */
	ADDB	x(SB), AL
/* Zo_m */
	POPL	x(SB)
/* Zr_m */
	ADDB	AL, x(SB)
/* Zrp_ */
	PUSHL	AX
/* Z_rp */
	POPL	AX
/* Z_ */
	POPAL
/* Zjmpb */
	JMP	out
/* Zloop */
out:
	LOOP	out
	RET

GLOBL	x(SB), $4
