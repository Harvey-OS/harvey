#define RARG R8

/* user-accessible CSRs */
#define CYCLO		0xc00

TEXT cycles(SB), 1, $0				/* cycles since power up */
	MOV	CSR(CYCLO), RARG
	RET
