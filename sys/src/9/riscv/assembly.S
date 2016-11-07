# main has to be in assembly because there's not telling
# what stack firmware has set up. It calls the
# bsp function with stack in a0 and configstring
# pointer in a1.
.globl bsp
.globl m0
.globl m0stack
.globl main
main:
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
	csrw	sscratch, a1
	mv	ra,a0
	mv	sp, a2
	mv 	a0, a2
	mv	a1, a3
	ret
# startmach starts a function after setting up the mach
# pointer. The function is in a0, the mach pointer in a1.
.globl startmach
startmach: 
	csrw	sscratch, a1
	mv	ra,a0
	mv	sp, a2
	mv 	a0, a2
	ret

.globl machp
machp:
	csrr	a0,sscratch
	ret


/*
 * Label consists of a stack pointer and a programme counter
 * 0(%rdi) is the SP, 8(%rdi) is the PC
 */
.global gotolabel
gotolabel:
	LD	sp,0(a0)
	LD	x2,16(a0)
	LD	a0,8(a0)
	MV	ra,a0
	RET

	/* save all registers on this stack, the save stack
	* in the label struct.
	*/
.global slim_setlabel
slim_setlabel:
	SD	sp,0(a0)
	SD	ra,8(a0)
	SD	x2,16(a0)
	RET


#if 0
.globl rdtsc
rdtsc:
	RDCYCLE a0
	RET
#endif
