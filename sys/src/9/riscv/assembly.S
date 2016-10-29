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


.globl rdtsc
rdtsc:
	RDCYCLE a0
	RET
