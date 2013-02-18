#include <sys/regdef.h>
#include <sys/asm.h>

LEAF(getcallerpc)
	.set	noreorder
	addiu	t0,a0,-4
	j	ra
	lw	v0,0(t0)
	.set reorder
	END(getcallerpc)
