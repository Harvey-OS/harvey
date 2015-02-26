/*
 * SIMD Floating Point.
 * Note: for x87 instructions which have both a 'wait' and 'nowait' version,
 * 8a only knows the 'wait' mnemonic but does NOT insertthe WAIT prefix byte
 * (i.e. they act like their FNxxx variations) so WAIT instructions must be
 * explicitly placed in the code if necessary.
 */
.globl _clts
_clts:
	CLTS
	RET

.globl _fldcw
_fldcw:
//	MOVQ	%rdi, cw+0(FP)
//	FLDCW	cw+0(FP)
	RET

.globl _fnclex
_fnclex:
	FCLEX
	RET

.globl _fninit
_fninit:
	FINIT					/* no WAIT */
	RET

.globl _fxrstor
_fxrstor:
	FXRSTOR64 0(%rdi)
	RET

.globl _fxsave
_fxsave:
	FXSAVE64 0(%rdi)
	RET

.globl _fwait
_fwait:
	WAIT
	RET

.globl _ldmxcsr
_ldmxcsr:
#warning "(FP)?"
//	MOVQ	%rdi, mxcsr+0(FP)
	//LDMXCSR	mxcsr+0(FP)
	RET

.globl _stts
_stts:
	MOVQ	%CR0, %rax
	ORQ	$8, %rax				/* Ts */
	MOVQ	%rax, %CR0
	RET
