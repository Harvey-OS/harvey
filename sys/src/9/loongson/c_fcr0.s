TEXT	C_fcr0(SB), $-4
	MOVW	$0x500, R1	/* claim to be an r4k, thus have ll/sc */
	RET
