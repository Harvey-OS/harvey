/*
 * to enable 32- and 64-bit kernels to use a single seek implementation,
 * we need to insert the address of the return value, on the stack
 * before the existing arguments (fd, where, whence) and after return
 * address at SP.  the trick is that we point the return address on the
 * stack at itself.
 */
TEXT seek(SB), 1, $-4
	SUB	$XLEN, R2		/* make room for &ret */
	MOVW	R8, (2*XLEN)(SP)	/* write fd to stack */
	MOV	$XLEN(SP), R8		/* address of ret */
	MOV	R8, XLEN(SP)		/* write &ret to stack */
	MOV	R1, 0(SP)		/* write return address to stack */
	MOV	$SYSCALL, R8
	ECALL
	BLT	R8, ret
	MOV	XLEN(SP), R8		/* okay: copy ret into RARG */
ret:
	MOV	0(SP), R1		/* restore return address */
	ADD	$XLEN, R2		/* pop return addr */
	RET
