TEXT	_tas(SB), $0
	SYNC
	MOVD	R3, R4
	MOVWZ	$0xdeaddead,R5
tas1:
/* taken out for the 755.  dcbf and L2 caching do not seem to get on
    with eachother.  It seems that dcbf is desctructive in the L2 cache 
    (also see l.s) */
//	DCBF	(R4)	
	SYNC
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	STWCCC	R5, (R4)
	BNE	tas1
tas0:
	SYNC
	ISYNC
	RETURN
