MODE $64

TEXT rdpmc(SB), 1, $-4				/* performance monitor counter */
	MOVL	RARG, CX
	RDPMC						/* read CX performance counter */
	XCHGL	DX, AX				/* swap lo/hi, zero-extend */
	SHLQ	$32, AX				/* hi<<32 */
	ORQ	DX, AX					/* (hi<<32)|lo */
	RET
