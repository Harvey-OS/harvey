TEXT _cycles(SB),1,$0				/* time stamp counter; cycles since power up */
	RDTSC
	MOVL	AX, 0(RARG)			/* lo */
	MOVL	DX, 4(RARG)			/* hi */
	RET
