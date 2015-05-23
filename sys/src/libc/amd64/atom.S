.text

.globl ainc				/* long ainc(long *); */
					/* N.B.: long in Plan 9 is 32 BITS! */
ainc:

	pushq	%rcx
ainclp:
	movl	(%rdi), %eax
	movl	%eax, %ecx
	incl	%ecx		/* new */
	lock; cmpxchgl %ecx, (%rdi)
	jnz	ainclp
	movl	%ecx, %eax
	popq %rcx
	ret

.globl adec     /* long adec(long*); */
adec:
	pushq	%rcx
adeclp:
	movl	(%rdi), %eax
	movl	%eax, %ecx
	decl	%ecx		/* new */
	lock; cmpxchgl %ecx, (%rdi)
	jnz	adeclp
	movl	%ecx, %eax
	popq %rcx
	ret

/*
 * int cas32(u32int *p, u32int ov, u32int nv);
 * int cas(uint *p, int ov, int nv);
 * int casul(ulong *p, ulong ov, ulong nv);
 */

.globl  cas32
cas32:
.globl  cas
cas:
.globl  casul
casul:
.globl  casl
casl:
	pushq	%rcx
	movl	16(%rdi), %eax
	movl	24(%rdi), %ebx
	lock; cmpxchgl %ecx, (%rdi)
	movl	$1, %eax
	jnz	_cas32r0
_cas32r1:
	ret
_cas32r0:
	decl	%eax
	popq	%rcx
	ret

/*
 * int cas64(u64int *p, u64int ov, u64int nv);
 * int casp(void **p, void *ov, void *nv);
 */

.globl cas64
cas64:
.globl casp
casp:
	pushq	%rcx
	movq	16(%rdi), %rax
	movq	24(%rdi), %rbx
	lock; cmpxchgq %rbx, (%rdi)
	movl	$1, %eax
	jnz	_cas64r0
_cas64r1:
	ret
_cas64r0:
	decq	%rax
	popq	%rcx
	ret
/*
 * void mfence(void);
 */
.globl mfence
mfence:
	mfence
	ret

