#define RDTSC 		BYTE $0x0F; BYTE $0x31

TEXT _cycles(SB),1,$0		/* time stamp counter; cycles since power up */
	RDTSC
	MOVL	vlong+0(FP), CX	/* &vlong */
	MOVL	AX, 0(CX)	/* lo */
	MOVL	DX, 4(CX)	/* hi */
	RET
