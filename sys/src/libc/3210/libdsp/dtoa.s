#define	OBUF	0x50030408

	TEXT	dtoa(SB), $0
stall:
	MOVW	$OBUF, R2
	BRA	OBF, stall
	MOVW	R3, (R2)
	RETURN
