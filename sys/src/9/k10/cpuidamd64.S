/*
 * The CPUID instruction is always supported on the amd64.
 */
.globl cpuid
cpuid:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	movl	%edi, -12(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movl	-12(%rbp), %eax

	cpuid 	/* Argument in %rax */

	movq	-24(%rbp), %rcx
	movl	%eax, (%rcx)
	movq	-32(%rbp), %rax
	movl	%edx, (%rax)
	popq	%rbx
	popq	%rbp
	ret

/*
 * Basic timing loop to determine CPU frequency.
 * The AAM instruction is not available in 64-bit mode.
 */
.globl aamloop
aamloop:
	movq	%rdi, %rcx
aaml1:
	xorq	%rax, %rax				/* close enough */
	loop	aaml1
	ret
