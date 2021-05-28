#define LINK	R1
#define SP	R2
#define ARG	8

#define FFLAGS		1
#define FRM		2
#define FCSR		3

TEXT	getfsr(SB), $0
	MOV	CSR(FCSR), R(ARG)
	RET

TEXT	setfsr(SB), $0
	MOV	R(ARG), CSR(FCSR)
	RET

TEXT	getfcr(SB), $0
	MOV	CSR(FCSR), R(ARG)
	RET

TEXT	setfcr(SB), $0
	MOV	R(ARG), CSR(FCSR)
	RET
