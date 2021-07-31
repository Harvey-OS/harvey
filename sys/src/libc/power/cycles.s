#define TBRL	268
#define TBRU	269		/* Time base Upper/Lower (Reading) */

TEXT cycles(SB),1,$0	/* time stamp counter; _cycles since power up */
loop:
	MOVW	SPR(TBRU),R7
	MOVW	SPR(TBRL),R8
	MOVW	SPR(TBRU),R5
	CMP		R5,R7
	BNE		loop
	MOVW	R7,0(R3)
	MOVW	R8,4(R3)
	RETURN
