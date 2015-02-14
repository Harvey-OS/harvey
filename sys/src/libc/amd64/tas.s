/*
 * The kernel and the libc use the same constant for TAS
 */
.text
.globl _tas
_tas:
	movl    $0xdeaddead, %eax
	xchgl   %eax, 0(%rdi)            /* lock->key */
	ret

