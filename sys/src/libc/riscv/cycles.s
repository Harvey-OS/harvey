#define RARG R8

/* user-accessible CSRs */
#define CYCLO		0xc00
#define CYCHI		0xc80			/* RV32 only */

/* cycles(&uvl) */
TEXT cycles(SB), 1, $0				/* cycles since power up */
again:
	MOV	CSR(CYCHI), R9
	MOV	CSR(CYCLO), R11
	MOV	CSR(CYCHI), R10
	BNE	R9, R10, again			/* wrapped? try again */
	MOV	R11, 0(RARG)
	MOV	R9, XLEN(RARG)
	RET
