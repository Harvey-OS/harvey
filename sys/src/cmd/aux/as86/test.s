/* test program for 8086 assembler */
/*
 * do we now have these types of comments?
 */

	.long	0
start:
	adcb	$1, al
	adcb	$1, (bp)
	adcb	cl, (bx)
	adcb	(bx), cl
	adc	$2, ax
	adc	$2, (bp)
	adc	cx, (bx)
	adc	(bx), cx

	addb	$1, al
	add	$2, ax
	addb	$4, ah
	add	$5, cx

Test:
	add	bx, dx
	add	bx, sp
	add	bx, bp
	add	bx, si
	add	bx, di

	call	start
	call	*start
	add	Data, ax

	add	ax, Stuff
	add	Stuff, ax
	add	ax, (bx)
	add	ax, (bp)
	add	ax, (si)
	add	ax, (di)
	add	ax, -10(bp)

	andb	$1, al
	andb	$1, (bp)
	andb	cl, (bx)
	andb	(bx), cl
	and	$2, ax
	and	$2, (bp)
	and	cx, (bx)
	and	(bx), cx
	and	$4, ax
	and	$4, (bx)
	and	cx, (bx)
	and	(bx), cx

	bswap	di

	cbw
	cwd

	clc
	cld
	cli

	cmc

	cmpb	$1, al
	cmpb	$1, (bp)
	cmpb	cl, (bx)
	cmpb	(bx), cl
	cmp	$2, ax
	cmp	$2, (bp)
	cmp	cx, (bx)
	cmp	(bx), cx
	cmp	$4, ax
	cmp	$4, (bx)
	cmp	cx, (bx)
	cmp	(bx), cx

	cpuid
	cwd
/*	cdq	*/

	decb	(bx)
	dec	bx
	dec	(bx)
	dec	di
	dec	(di)

	divb	(di)
	div	(di)

here:
	jmp	here
	jmp	*here
	jmp	(di)

	halt

	idivb	(di)
	idiv	(di)

	inb	$2
	inb
	in	$2
	in
	in	$2
	in

	incb	(bx)
	inc	bx
	inc	(bx)
	inc	di
	inc	(di)

	insb
	ins

	int3
	int	$2
	into

	invd
	invlpg	(bx)
	iret

l:
	ja	l
	jae	l
	jb	l
	jbe	l
	jc	l
	jcxz	l
/*	jecxz	l	*/
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
	lea	(bx), di
	lodsb
	lods

m:
	loop	m
	loope	m
	loopz	m
	loopne	m
	loopnz	m

	movb	$1, ah
	movb	$1, (bx)
	movb	(bx), dh
	movb	dh, (bx)

	mov	$2, cx
	mov	$2, (bx)
	mov	(bx), di
	mov	di, (bx)

	mov	$4, cx
	mov	$4, (bx)
	mov	(bx), di
	mov	di, (bx)

	mov	ss, di
	mov	di, ss

/*	mov	dr3, di
	mov	di, dr3

	mov	cr3, di
	mov	di, cr3		*/

	movsb
	movs

	movsbw	(bx), di
	movsbl	(bx), di

	movzbw	(bx), di
	movzbl	(bx), di

	mulb	(bx)
	mul	(bx)

	negb	(bx)
	neg	(bx)

	nop

	notb	(bx)
	notw	(bx)
	notl	(bx)

	orb	$1, al
	orb	$1, (bp)
	orb	cl, (bx)
	orb	(bx), cl
	or	$2, ax
	or	$2, (bp)
	or	cx, (bx)
	or	(bx), cx
	or	$4, ax
	or	$4, (bx)
	or	cx, (bx)
	or	(bx), cx

	outb	$1
	outb
	out	$2
	out
	out	$4
	out

	pop	(bx)
	pop	(bx)
	pop	di
	pop	di
	pop	ds
	pop	es
	pop	ss
	pop	fs

	popa
	popf

	push	(bx)
	push	(bx)
	push	di
	push	di
	push	$4
	push	cs
	push	ss
	push	ds
	push	es
	push	fs

	pusha
	pushf

	rclb	$1, ah
	rclb	cl, ah
	rclb	$2, ah
	rcl	$1, bx
	rcl	cl, bx
	rcl	$2, bx
	rcl	$1, di
	rcl	cl, di
	rcl	$2, di

	rcrb	$1, ah
	rcrb	cl, ah
	rcrb	$2, ah
	rcr	$1, bx
	rcr	cl, bx
	rcr	$2, bx
	rcr	$1, di
	rcr	cl, di
	rcr	$2, di

	rolb	$1, ah
	rolb	cl, ah
	rolb	$2, ah
	rol	$1, bx
	rol	cl, bx
	rol	$2, bx
	rol	$1, di
	rol	cl, di
	rol	$2, di

	rorb	$1, ah
	rorb	cl, ah
	rorb	$2, ah
	ror	$1, bx
	ror	cl, bx
	ror	$2, bx
	ror	$1, di
	ror	cl, di
	ror	$2, di

	rep
	repe
	repne

	ret
	ret	$4
	lret
	lret	$4

	sahf

	salb	$1, ah
	salb	cl, ah
	salb	$2, ah
	sal	$1, bx
	sal	cl, bx
	sal	$2, bx
	sal	$1, di
	sal	cl, di
	sal	$2, di

	sarb	$1, ah
	sarb	cl, ah
	sarb	$2, ah
	sar	$1, bx
	sar	cl, bx
	sar	$2, bx
	sar	$1, di
	sar	cl, di
	sar	$2, di

	shlb	$1, ah
	shlb	cl, ah
	shlb	$2, ah
	shl	$1, bx
	shl	cl, bx
	shl	$2, bx
	shl	$1, di
	shl	cl, di
	shl	$2, di

	shrb	$1, ah
	shrb	cl, ah
	shrb	$2, ah
	shr	$1, bx
	shr	cl, bx
	shr	$2, bx
	shr	$1, di
	shr	cl, di
	shr	$2, di

	sbbb	$1, al
	sbbb	$1, (bp)
	sbbb	cl, (bx)
	sbbb	(bx), cl
	sbb	$2, ax
	sbb	$2, (bp)
	sbb	cx, (bx)
	sbb	(bx), cx
	sbb	$4, ax
	sbb	$4, (bx)
	sbb	cx, (bx)
	sbb	(bx), cx

	scasb
	scas

	seta	(bx)
	setae	(bx)
	setb	(bx)
	setbe	(bx)
	setc	(bx)
	sete	(bx)
	setg	(bx)
	setge	(bx)
	setl	(bx)
	setle	(bx)
	setna	(bx)
	setnae	(bx)
	setnb	(bx)
	setnbe	(bx)
	setnc	(bx)
	setne	(bx)
	setng	(bx)
	setnge	(bx)
	setnl	(bx)
	setnle	(bx)
	setno	(bx)
	setnp	(bx)
	setns	(bx)
	setnz	(bx)
	seto	(bx)
	setp	(bx)
	setpe	(bx)
	setpo	(bx)
	sets	(bx)
	setz	(bx)

	stc
	std
	sti

	stosb
	stos

	str	(bx)

	subb	$1, al
	subb	$1, (bp)
	subb	cl, (bx)
	subb	(bx), cl
	sub	$2, ax
	sub	$2, (bp)
	sub	cx, (bx)
	sub	(bx), cx
	sub	$4, ax
	sub	$4, (bx)
	sub	cx, (bx)
	sub	(bx), cx

	testb	$1, al
	testb	$1, (bp)
	testb	cl, (bx)
	test	$2, ax
	test	$2, (bp)
	test	cx, (bx)
	test	$4, ax
	test	$4, (bx)
	test	cx, (bx)

	xchgb	ah, (bx)
	xchgb	(bx), ah
	xchgl	ax, bx
	xchgl	bx, ax
	xchgl	dx, (bx)
	xchgl	(bx), dx
	xchgl	ax, bx
	xchgl	bx, ax
	xchgl	dx, (bx)
	xchgl	(bx), dx
	
	xlatb

	xorb	$1, al
	xorb	$1, (bp)
	xorb	cl, (bx)
	xorb	(bx), cl
	xorl	$4, ax
	xorl	$4, (bx)
	xorl	cx, (bx)
	xorl	(bx), cx

	.bss
	.space	100
Data:
Stuff:	.long	0
	.text
