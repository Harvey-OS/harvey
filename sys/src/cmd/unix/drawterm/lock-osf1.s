	.set noat
	.set noreorder
	.text
	.arch	generic
	.align 4
	.globl  tas
	.ent 	tas
tas:												 	.frame  $sp, 0, $26
	.prologue 0
	mov	$16, $1		# $16 appears to be argument

tas1:
	ldl_l	$0, ($1)		# $0 appears to be return register
	bne	$0, tas2

	mov	1, $2
	stl_c	$2, ($1)
	beq	$2, tas1
	
tas2:
	ret	($26)		# $26 appears to be link register
	.end 	tas


	# this is the plan 9 version, tossed into an array and then disassembled with gdb.
	#
	# this is basically the goal for the above code when we disassemble it in gdb.
	#
	# 0x140000010 <i>:        or      v0,zero,t0
	# 0x140000014 <i+4>:      ldl_l   v0,0(t0)
	# 0x140000018 <i+8>:     bne     v0,0x140000030 <i+24>
	# 0x14000001c <i+12>:     lda     t1,1(zero)
	# 0x140000020 <i+16>:     stl_c   t1,0(t0)
	# 0x140000024 <i+20>:     beq     t1,0x140000028 <i+4>
	# 0x140000028 <i+24>:     jmp     zero,(ra),0x140000044	# return
