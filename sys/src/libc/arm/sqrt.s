/* VFP VSQRTD instruction, F1 to F0 */
#define VSQRTD WORD $0xeeb10bc1

	TEXT	sqrt(SB), 1, $0
	MOVD	a+0(FP),F1
	VSQRTD
	RET
