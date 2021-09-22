#define ARG	8

#define FFLAGS		1
#define FRM		2
#define FCSR		3

TEXT	getfcr(SB), $-4
TEXT	getfsr(SB), $-4
	MOV	CSR(FCSR), R(ARG)
	RET

TEXT	setfcr(SB), $-4
TEXT	setfsr(SB), $-4
	MOV	R(ARG), CSR(FCSR)
	RET
