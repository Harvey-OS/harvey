.globl	sqrt
sqrt:
	movsd	16(%rdi), %xmm0
	sqrtsd	%xmm0, %xmm0
	ret

