# Flagging boottext to be text.  Check out:
# http://sourceware.org/binutils/docs/as/Section.html
.section .boottext, "awx"

.globl _start
_start:
	mrs	x1, mpidr_el1	// Stop all but main core
	and	x1, x1, #3
	cbz	x1, _maincore

_haltcore:
	wfe			// Not main core
	b	_haltcore

_maincore:
	ldr	x1, =_start	// Prepare stack
	mov	sp, x1

	ldr	x1, =main
	br	x1		// Jump to main in kernel half
