	.section	.bss
	.align	4
.L4_.bss:
	.align	4
Solaris_Asm_IntP: / Offset 0
	.type	Solaris_Asm_IntP,@object
	.size	Solaris_Asm_IntP,4
	.set	.,.+4
Solaris_Asm_VoidP: / Offset 4
	.type	Solaris_Asm_VoidP,@object
	.size	Solaris_Asm_VoidP,4
	.set	.,.+4
	.section	.data
	.align	4
.L2_.data:
	.align	4
.L01:	.string	"canlock corrupted 0x%lux\n\000"
	.set	.,.+0x2
	.section	.text
	.align	4
.L1_.text:

	.align	4
	.align	4
	.globl	canlock
canlock:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	movl	8(%ebp),%eax
	movl	%eax,Solaris_Asm_IntP
	movl	$1, %ecx
	xchg	(%eax), %ecx
	andl	%ecx,%ecx
	jne	.L1
	movl	$1,%esi
	jmp	.L2
	.align	4
.L3:
	subl	%esi,%esi
	jmp	.L2
	.align	4
.L1:
	cmpl	$1,%ecx
	je	.L3
	pushl	%ecx
	pushl	$.L01
	call	print
	addl	$8,%esp
.L2:
	movl	%esi,%eax
	popl	%esi
	leave	
	ret	
	.type	canlock,@function
	.size	canlock,.-canlock
