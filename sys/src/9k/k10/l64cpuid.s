/*
 * The CPUID instruction is always supported on the amd64.
 */
TEXT cpuid(SB), $-4
	MOVL	RARG, AX			/* function in AX */
	MOVLQZX	cx+8(FP), CX			/* iterator/index/etc. */

	CPUID

	MOVQ	info+16(FP), BP
	MOVL	AX, 0(BP)
	MOVL	BX, 4(BP)
	MOVL	CX, 8(BP)
	MOVL	DX, 12(BP)
	RET

/*
 * Basic timing loop to determine CPU frequency.
 * The AAM instruction is not available in 64-bit mode.
 */
TEXT aamloop(SB), 1, $-4
	MOVLQZX	RARG, CX
aaml1:
	XORQ	AX, AX				/* close enough */
	LOOP	aaml1
	RET
