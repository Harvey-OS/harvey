#include <sys/regdef.h>
#include <sys/asm.h>

LEAF(getcallerpc)
	.set	noreorder
	lw	v0, 28(sp)
	j	ra
	.set reorder
	END(getcallerpc)
