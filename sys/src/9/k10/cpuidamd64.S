/*
 * The CPUID instruction is always supported on the amd64.
 */
.globl cpuid
cpuid:
	MOVQ	%rdi, %rax

	CPUID					/* argument in %rax */

	MOVQ	%rsi, %rbp
#warning "multipel return"
//	MOVL	%rax, 0(%rpb)
//	MOVL	BX, 4(%rpb)
//	MOVL	CX, 8(%rpb)
//	MOVL	DX, 12(%rpb)
	RET

/*
 * Basic timing loop to determine CPU frequency.
 * The AAM instruction is not available in 64-bit mode.
 */
.globl aamloop
aamloop:
	MOVQ	%rdi, %rcx
aaml1:
	XORQ	%rax, %rax				/* close enough */
	LOOP	aaml1
	RET
