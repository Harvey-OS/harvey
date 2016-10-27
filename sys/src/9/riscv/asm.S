# startmach starts a function after setting up the mach
# pointer. The function is in a0, the mach pointer in a1.
.globl startmach
startmach: 
	csrw	sscratch, a1
	mv	ra,a0
	ret

.globl machp
machp:
	csrr	a0,sscratch
	ret
