/* test program for assembler */
/*
 * do we now have these types of comments?
 */

	.long	0
start:
	adcb	$1, %al
	adcb	$1, (%ebp)
	adcb	%cl, (%ebx)
	adcb	(%ebx), %cl
	adcw	$2, %ax
	adcw	$2, (%ebp)
	adcw	%cx, (%ebx)
	adcw	(%ebx), %cx
	adcl	$4, %eax
	adcl	$4, (%ebx)
	adcl	%ecx, (%ebx)
	adcl	(%ebx), %ecx

	addb	$1, %al
	addw	$2, %ax
	addl	$3, %eax
	addb	$4, %ah
	addw	$5, %cx
	addl	$6, %ecx

Test:
	addl	%ebx, %edx
	addl	%ebx, %esp
	addl	%ebx, %ebp
	addl	%ebx, %esi
	addl	%ebx, %edi

	call	start
	call	*start
	calls	start
	calls	*start
	addl	Data, %eax

	addl	%eax, Stuff
	addl	Stuff, %eax
	addl	%eax, (%eax)
	addl	%eax, (%ecx)
	addl	%eax, (%edx)
	addl	%eax, (%ebx)
	addl	%eax, (%esp)
	addl	%eax, (%ebp)
	addl	%eax, (%esi)
	addl	%eax, (%edi)
	addl	%eax, -10(%ebp)
	addl	%eax, 4(%ebp, %ecx)
	addl	%eax, 4(%ebp, %ecx, 2)
	addl	%eax, 4(%ebp, %ecx, 4)
	addl	%eax, 4(%ebp, %ecx, 8)
	addl	4(%ebp, %ecx, 8), %eax

	andb	$1, %al
	andb	$1, (%ebp)
	andb	%cl, (%ebx)
	andb	(%ebx), %cl
	andw	$2, %ax
	andw	$2, (%ebp)
	andw	%cx, (%ebx)
	andw	(%ebx), %cx
	andl	$4, %eax
	andl	$4, (%ebx)
	andl	%ecx, (%ebx)
	andl	(%ebx), %ecx

	bswap	%edi

	cbw
	cwde

	clc
	cld
	cli

	cmc

	cmpb	$1, %al
	cmpb	$1, (%ebp)
	cmpb	%cl, (%ebx)
	cmpb	(%ebx), %cl
	cmpw	$2, %ax
	cmpw	$2, (%ebp)
	cmpw	%cx, (%ebx)
	cmpw	(%ebx), %cx
	cmpl	$4, %eax
	cmpl	$4, (%ebx)
	cmpl	%ecx, (%ebx)
	cmpl	(%ebx), %ecx

	cpuid
	cwd
	cdq

	decb	(%ebx)
	decw	%ebx
	decw	(%ebx)
	decl	%edi
	decl	(%edi)

	divb	(%edi)
	divw	(%edi)
	divl	(%edi)

here:
	jmp	here
	jmp	*here
	jmp	(%edi)

	halt

	idivb	(%edi)
	idivw	(%edi)
	idivl	(%edi)

	inb	$2
	inb
	inw	$2
	inw
	inl	$2
	inl

	incb	(%ebx)
	incw	%ebx
	incw	(%ebx)
	incl	%edi
	incl	(%edi)

	insb
	insw
	insl

	int3
	int	$2
	into

	invd
	invlpg	(%ebx)
	iret

l:
	ja	l
	jae	l
	jb	l
	jbe	l
	jc	l
	jcxz	l
	jecxz	l
	je	l
	jg	l
	jge	l
	jl	l
	jle	l
	jna	l
	jnae	l
	jnb	l
	jnbe	l
	jnc	l
	jne	l
	jng	l
	jnge	l
	jnl	l
	jnle	l
	jno	l
	jnp	l
	jns	l
	jnz	l
	jo	l
	jp	l
	jpe	l
	jpo	l
	js	l
	jz	l

	jmps	m
	jmp	Test
	jmp	*Test

	lahf
	leal	(%ebx), %edi
	lodsb
	lodsw
	lodl

m:
	loop	m
	loope	m
	loopz	m
	loopne	m
	loopnz	m

	movb	$1, %ah
	movb	$1, (%ebx)
	movb	(%ebx), %dh
	movb	%dh, (%ebx)

	movw	$2, %cx
	movw	$2, (%ebx)
	movw	(%ebx), %di
	movw	%di, (%ebx)

	movl	$4, %ecx
	movl	$4, (%ebx)
	movl	(%ebx), %edi
	movl	%edi, (%ebx)

	movw	%ss, %edi
	movw	%edi, %ss

	movl	%dr3, %edi
	movl	%edi, %dr3

	movl	%cr3, %edi
	movl	%edi, %cr3

	movsb
	movsw
	movsl

	movsbw	(%ebx), %edi
	movsbl	(%ebx), %edi
	movswl	(%ebx), %edi

	movzbw	(%ebx), %edi
	movzbl	(%ebx), %edi
	movzwl	(%ebx), %edi

	mulb	(%ebx)
	mulw	(%ebx)
	mull	(%ebx)

	negb	(%ebx)
	negw	(%ebx)
	negl	(%ebx)

	nop

	notb	(%ebx)
	notw	(%ebx)
	notl	(%ebx)

	orb	$1, %al
	orb	$1, (%ebp)
	orb	%cl, (%ebx)
	orb	(%ebx), %cl
	orw	$2, %ax
	orw	$2, (%ebp)
	orw	%cx, (%ebx)
	orw	(%ebx), %cx
	orl	$4, %eax
	orl	$4, (%ebx)
	orl	%ecx, (%ebx)
	orl	(%ebx), %ecx

	outb	$1
	outb
	outw	$2
	outw
	outl	$4
	outl

	popw	(%ebx)
	popl	(%ebx)
	popw	%di
	popl	%edi
	popl	%ds
	popl	%es
	popl	%ss
	popl	%fs
	popl	%gs

	popa
	popf

	pushw	(%ebx)
	pushl	(%ebx)
	pushw	%di
	pushl	%edi
	pushl	$4
	pushl	%cs
	pushl	%ss
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	pusha
	pushf

	rclb	$1, %ah
	rclb	%cl, %ah
	rclb	$2, %ah
	rclw	$1, %bx
	rclw	%cl, %bx
	rclw	$2, %bx
	rcll	$1, %edi
	rcll	%cl, %edi
	rcll	$2, %edi

	rcrb	$1, %ah
	rcrb	%cl, %ah
	rcrb	$2, %ah
	rcrw	$1, %bx
	rcrw	%cl, %bx
	rcrw	$2, %bx
	rcrl	$1, %edi
	rcrl	%cl, %edi
	rcrl	$2, %edi

	rolb	$1, %ah
	rolb	%cl, %ah
	rolb	$2, %ah
	rolw	$1, %bx
	rolw	%cl, %bx
	rolw	$2, %bx
	roll	$1, %edi
	roll	%cl, %edi
	roll	$2, %edi

	rorb	$1, %ah
	rorb	%cl, %ah
	rorb	$2, %ah
	rorw	$1, %bx
	rorw	%cl, %bx
	rorw	$2, %bx
	rorl	$1, %edi
	rorl	%cl, %edi
	rorl	$2, %edi

	rep
	repe
	repne

	ret
	ret	$4
	lret
	lret	$4

	sahf

	salb	$1, %ah
	salb	%cl, %ah
	salb	$2, %ah
	salw	$1, %bx
	salw	%cl, %bx
	salw	$2, %bx
	sall	$1, %edi
	sall	%cl, %edi
	sall	$2, %edi

	sarb	$1, %ah
	sarb	%cl, %ah
	sarb	$2, %ah
	sarw	$1, %bx
	sarw	%cl, %bx
	sarw	$2, %bx
	sarl	$1, %edi
	sarl	%cl, %edi
	sarl	$2, %edi

	shlb	$1, %ah
	shlb	%cl, %ah
	shlb	$2, %ah
	shlw	$1, %bx
	shlw	%cl, %bx
	shlw	$2, %bx
	shll	$1, %edi
	shll	%cl, %edi
	shll	$2, %edi

	shrb	$1, %ah
	shrb	%cl, %ah
	shrb	$2, %ah
	shrw	$1, %bx
	shrw	%cl, %bx
	shrw	$2, %bx
	shrl	$1, %edi
	shrl	%cl, %edi
	shrl	$2, %edi

	sbbb	$1, %al
	sbbb	$1, (%ebp)
	sbbb	%cl, (%ebx)
	sbbb	(%ebx), %cl
	sbbw	$2, %ax
	sbbw	$2, (%ebp)
	sbbw	%cx, (%ebx)
	sbbw	(%ebx), %cx
	sbbl	$4, %eax
	sbbl	$4, (%ebx)
	sbbl	%ecx, (%ebx)
	sbbl	(%ebx), %ecx

	scasb
	scasw
	scasl

	seta	(%ebx)
	setae	(%ebx)
	setb	(%ebx)
	setbe	(%ebx)
	setc	(%ebx)
	sete	(%ebx)
	setg	(%ebx)
	setge	(%ebx)
	setl	(%ebx)
	setle	(%ebx)
	setna	(%ebx)
	setnae	(%ebx)
	setnb	(%ebx)
	setnbe	(%ebx)
	setnc	(%ebx)
	setne	(%ebx)
	setng	(%ebx)
	setnge	(%ebx)
	setnl	(%ebx)
	setnle	(%ebx)
	setno	(%ebx)
	setnp	(%ebx)
	setns	(%ebx)
	setnz	(%ebx)
	seto	(%ebx)
	setp	(%ebx)
	setpe	(%ebx)
	setpo	(%ebx)
	sets	(%ebx)
	setz	(%ebx)

	stc
	std
	sti

	stosb
	stosw
	stosl

	str	(%ebx)

	subb	$1, %al
	subb	$1, (%ebp)
	subb	%cl, (%ebx)
	subb	(%ebx), %cl
	subw	$2, %ax
	subw	$2, (%ebp)
	subw	%cx, (%ebx)
	subw	(%ebx), %cx
	subl	$4, %eax
	subl	$4, (%ebx)
	subl	%ecx, (%ebx)
	subl	(%ebx), %ecx

	testb	$1, %al
	testb	$1, (%ebp)
	testb	%cl, (%ebx)
	testw	$2, %ax
	testw	$2, (%ebp)
	testw	%cx, (%ebx)
	testl	$4, %eax
	testl	$4, (%ebx)
	testl	%ecx, (%ebx)

	xchgb	%ah, (%ebx)
	xchgb	(%ebx), %ah
	xchgw	%ax, %ebx
	xchgw	%ebx, %ax
	xchgw	%dx, (%ebx)
	xchgw	(%ebx), %dx
	xchgl	%eax, %ebx
	xchgl	%ebx, %eax
	xchgl	%edx, (%ebx)
	xchgl	(%ebx), %edx
	
	xlatb

	xorb	$1, %al
	xorb	$1, (%ebp)
	xorb	%cl, (%ebx)
	xorb	(%ebx), %cl
	xorw	$2, %ax
	xorw	$2, (%ebp)
	xorw	%cx, (%ebx)
	xorw	(%ebx), %cx
	xorl	$4, %eax
	xorl	$4, (%ebx)
	xorl	%ecx, (%ebx)
	xorl	(%ebx), %ecx

	.bss
	.space	100
Data:
	.text
