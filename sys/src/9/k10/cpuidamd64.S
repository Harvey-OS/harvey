/*
 * The CPUID instruction is always supported on the amd64.
 */
.globl cpuid
cpuid:
	pushq %rbx
	pushq %rcx
	pushq %rdx
	movq	%rdi, %rax
	movq	%rsi, %rcx
	pushq	%r15
	movq	%rdx, %r15

	cpuid 	/* Argument in %rax */

	movl	%ebx, 0(%r15)
	movl	%ecx, 8(%r15)
	movl	%edx, 24(%r15)
	popq	%r15
	popq	%rdx
	popq	%rcx
	pop	%rbx
	ret

/*
 * Basic timing loop to determine CPU frequency.
 * The AAM instruction is not available in 64-bit mode.
 */
.globl aamloop
aamloop:
	pushq	%rcx
	movq	%rdi, %rcx
aaml1:
	xorq	%rax, %rax				/* close enough */
	loop	aaml1
	popq	%rcx
	ret
