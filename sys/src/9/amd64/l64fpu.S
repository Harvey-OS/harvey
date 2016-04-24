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
	ret

.globl _fldcw
_fldcw:
	fldcw	(%rdi)
	ret

.globl _fnclex
_fnclex:
	FCLEX
	ret

.globl _fninit
_fninit:
	FINIT					/* no WAIT */
	ret

.globl _fxrstor
_fxrstor:
	// Damn. clang lags on assembly, badly.
	//FXRSTOR64 0(%rdi)
	.byte 0x48, 0x0f, 0xae, 0x0f //          	fxrstor64 (%rdi)
	ret

.globl _fxsave
_fxsave:
	//FXSAVE64 0(%rdi)
	.byte 0x48, 0x0f, 0xae, 0x07 //          	fxsave64 (%rdi)
	ret

.globl _fwait
_fwait:
	WAIT
	ret

.globl _ldmxcsr
_ldmxcsr:
	LDMXCSR	(%rdi)
	ret

.globl _stts
_stts:
	MOVQ	%CR0, %rax
	ORQ	$8, %rax				/* Ts */
	MOVQ	%rax, %CR0
	ret
