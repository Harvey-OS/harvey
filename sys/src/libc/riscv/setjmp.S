/* int setjmp (jmp_buf);  */
  .globl  setjmp
setjmp:
	SD	ra,0(a0) // X1
	SD	sp,8(a0) // X14
	SD	s0,16(a0) // X2
	// X0 is zero
	// X1 and X2 are done.
	sd x3, 48(a0)
	sd x4, 56(a0)
	sd x5, 64(a0)
	sd x6, 72(a0)
	sd x7, 80(a0)
	sd x8, 88(a0)
	sd x9, 96(a0)
	sd x10, 104(a0) // store a zero in jmp_buf
	sd x11, 112(a0) // Don't touch a1.
	sd x12, 120(a0)
	sd x13, 128(a0)
	// X14 done already
	sd x15, 144(a0)
	sd x16, 152(a0)
	sd x17, 160(a0)
	sd x18, 168(a0)
	sd x19, 176(a0)
	sd x20, 184(a0)
	sd x21, 192(a0)
	sd x22, 200(a0)
	sd x23, 208(a0)
	sd x24, 216(a0)
	sd x25, 224(a0)
	sd x26, 232(a0)
	sd x27, 240(a0)
	sd x28, 248(a0)
	sd x29, 256(a0)
	sd x30, 264(a0)
	sd x31, 272(a0)
	mv	a0, x0
	RET

/* volatile void longjmp (jmp_buf, int);  */
  .globl  longjmp
longjmp:
	LD	sp,8(a0) // X14
	LD	s0,16(a0) // X2
	// X0 is zero
	// X1 and X2 are done.
	LD x3, 48(a0)
	// Don't restore X4, that's reserved for Mach *
	//LD x4, 56(a0)
	LD x5, 64(a0)
	LD x6, 72(a0)
	LD x7, 80(a0)
	LD x8, 88(a0)
	LD x9, 96(a0)
	//LD x10, 104(a0) this is a0
	// LD x11, 112(a0)
	LD x12, 120(a0)
	LD x13, 128(a0)
	// X14 done already
	LD x15, 144(a0)
	LD x16, 152(a0)
	LD x17, 160(a0)
	LD x18, 168(a0)
	LD x19, 176(a0)
	LD x20, 184(a0)
	LD x21, 192(a0)
	LD x22, 200(a0)
	LD x23, 208(a0)
	LD x24, 216(a0)
	LD x25, 224(a0)
	LD x26, 232(a0)
	LD x27, 240(a0)
	LD x28, 248(a0)
	LD x29, 256(a0)
	LD x30, 264(a0)
	LD x30, 272(a0)
	LD	a0,0(a0)
	MV	ra,a0 // X1 (non zero by definition I hope.
	mv	a0, a1
	RET


