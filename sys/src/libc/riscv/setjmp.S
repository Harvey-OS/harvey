// We're going to assume 64 bits and hard float.
// if you want 32 bits, don't do this nonsense.
// Do it the plan 9 way: make a riscv32 directory
// and fix things in there. 
# define SZREG 8

/* int setjmp (jmp_buf);  */
  .globl  setjmp
setjmp:
	sd ra,  0*SZREG(a0)
	sd s0,  1*SZREG(a0)
	sd s1,  2*SZREG(a0)
	sd s2,  3*SZREG(a0)
	sd s3,  4*SZREG(a0)
	sd s4,  5*SZREG(a0)
	sd s5,  6*SZREG(a0)
	sd s6,  7*SZREG(a0)
	sd s7,  8*SZREG(a0)
	sd s8,  9*SZREG(a0)
	sd s9, 10*SZREG(a0)
	sd s10,11*SZREG(a0)
	sd s11,12*SZREG(a0)
	sd sp, 13*SZREG(a0)

	frsr a3

	fsd   fs0, 16*SZREG+ 0*8(a0)
	fsd   fs1, 16*SZREG+ 1*8(a0)
	fsd   fs2, 16*SZREG+ 2*8(a0)
	fsd   fs3, 16*SZREG+ 3*8(a0)
	fsd   fs4, 16*SZREG+ 4*8(a0)
	fsd   fs5, 16*SZREG+ 5*8(a0)
	fsd   fs6, 16*SZREG+ 6*8(a0)
	fsd   fs7, 16*SZREG+ 7*8(a0)
	fsd   fs8, 16*SZREG+ 8*8(a0)
	fsd   fs9, 16*SZREG+ 9*8(a0)
	fsd   fs10,16*SZREG+10*8(a0)
	fsd   fs11,16*SZREG+11*8(a0)

	sd a3, 15*SZREG(a0)

	li    a0, 0
	ret

/* volatile void longjmp (jmp_buf, int);  */
  .globl  longjmp
longjmp:
	ld ra,  0*SZREG(a0)
	ld s0,  1*SZREG(a0)
	ld s1,  2*SZREG(a0)
	ld s2,  3*SZREG(a0)
	ld s3,  4*SZREG(a0)
	ld s4,  5*SZREG(a0)
	ld s5,  6*SZREG(a0)
	ld s6,  7*SZREG(a0)
	ld s7,  8*SZREG(a0)
	ld s8,  9*SZREG(a0)
	ld s9, 10*SZREG(a0)
	ld s10,11*SZREG(a0)
	ld s11,12*SZREG(a0)
	ld sp, 13*SZREG(a0)

	ld a3, 15*SZREG(a0)

	fld  fs0, 16*SZREG+ 0*8(a0)
	fld  fs1, 16*SZREG+ 1*8(a0)
	fld  fs2, 16*SZREG+ 2*8(a0)
	fld  fs3, 16*SZREG+ 3*8(a0)
	fld  fs4, 16*SZREG+ 4*8(a0)
	fld  fs5, 16*SZREG+ 5*8(a0)
	fld  fs6, 16*SZREG+ 6*8(a0)
	fld  fs7, 16*SZREG+ 7*8(a0)
	fld  fs8, 16*SZREG+ 8*8(a0)
	fld  fs9, 16*SZREG+ 9*8(a0)
	fld  fs10,16*SZREG+10*8(a0)

	fssr a3

	seqz a0, a1
	add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
	ret

