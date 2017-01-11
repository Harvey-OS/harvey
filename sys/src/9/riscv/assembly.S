# main has to be in assembly because there's not telling
# what stack firmware has set up. It calls the
# bsp function with stack in a0 and configstring
# pointer in a1.
.text
.globl bsp
.globl m0
.globl m0stack
.globl main
main:
	csrw sscratch, x0
	mv	a3, a0
        lui     a5,%hi(bsp)
        add     a5,a5,%lo(bsp)
        mv      a0,a5
	//li	a1, $m0
        lui     a5,%hi(m0)
        add     a5,a5,%lo(m0)
        mv      a1,a5
	//li	a2, $m0stack
        lui     a5,%hi(m0stack)
        add     a5,a5,%lo(m0stack)
	li	a6, 32768
	add 	a5, a5, a6
        mv      a2,a5
	mv	tp, a1
	mv	ra,a0
	mv	sp, a2
	mv 	a0, a2
	mv	a1, a3
	ret
# startmach starts a function after setting up the mach
# pointer. The function is in a0, the mach pointer in a1.
.globl startmach
startmach:
	csrw sscratch, x0
	mv	tp, a1
	mv	ra,a0
	mv	sp, a2
	mv 	a0, a2
	ret

.globl machp
machp:
	mv	a0,tp
	ret


/*
 * Label consists of a stack pointer and a programme counter
 * 0(%rdi) is the SP, 8(%rdi) is the PC
 */
.global gotolabel
gotolabel:
	LD	sp,0(a0) // X14
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
	LD x11, 112(a0)
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
	LD	a0,8(a0)
	MV	ra,a0 // X1 (non zero by definition I hope.
	RET

	/* save all registers on this stack, the save stack
	* in the label struct.
	*/
.global slim_setlabel
slim_setlabel:
	// don't save X0 of course. Or x31
	/* not so slim! */
	SD	sp,0(a0) // X14
	SD	ra,8(a0) // X1
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
	sd x10, 104(a0)
	sd x11, 112(a0)
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
	mv	a0,zero
	RET


.globl stack
stack:
	mv a0, sp
	ret
#if 0
.globl rdtsc
rdtsc:
	RDCYCLE a0
	RET
#endif
/* set up uregs, then jump to a0 from uregs */
.globl touser
touser:
	csrw sscratch, tp
	LD	sp,8(a0) // X14
	LD	s0,16(a0) // X2
	// X0 is zero
	// X1 and X2 are done.
	LD x3, 48(a0)
	LD x4, 56(a0)
	LD x5, 64(a0)
	LD x6, 72(a0)
	LD x7, 80(a0)
	LD x8, 88(a0)
	LD x9, 96(a0)
	// a0 and a1 are restored below.

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
	LD	a1,0(a0)
	/* Jump to a0 in S-mode */
	//csrr t0, mstatus
	//li t1, ~(1<<8) // clear SPP
	//and t0, t0, t1
	//csrw mstatus, t0
	csrw sepc, a1
	LD a1, 112(a0)
	LD a0, 104(a0)
	.long 0x10200073
	sret

.globl sbi_set_mtimecmp
sbi_set_mtimecmp:
	tail -1888
