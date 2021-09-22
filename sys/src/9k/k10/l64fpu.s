/*
 * SIMD Floating Point.
 * Note: for x87 instructions which have both a 'wait' and 'nowait' version,
 * 8a only knows the 'wait' mnemonic but does NOT insert the WAIT prefix byte
 * (i.e. they act like their FNxxx variations) so WAIT instructions must be
 * explicitly placed in the code if necessary.
 */
#include "amd64l.h"

TEXT _clts(SB), 1, $-4
	CLTS
	RET

TEXT _fldcw(SB), 1, $-4				/* Load x87 FPU Control Word */
	MOVQ	RARG, cw+0(FP)
	FLDCW	cw+0(FP)
	RET

TEXT _fnclex(SB), 1, $-4
	FCLEX
	RET

TEXT _fninit(SB), 1, $-4
	FINIT					/* no WAIT */
	RET

TEXT _fxrstor(SB), 1, $-4
	FXRSTOR64 (RARG)
	RET

TEXT _fxsave(SB), 1, $-4
	FXSAVE64 (RARG)
	RET

TEXT _fwait(SB), 1, $-4
	WAIT
	RET

TEXT _ldmxcsr(SB), 1, $-4			/* Load MXCSR */
	MOVQ	RARG, mxcsr+0(FP)
	LDMXCSR	mxcsr+0(FP)
	RET

TEXT _stts(SB), 1, $-4
	MOVQ	CR0, AX
	ORQ	$Ts, AX
	MOVQ	AX, CR0
	RET
