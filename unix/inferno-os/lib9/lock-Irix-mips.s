#include <sys/regdef.h>
#include <sys/asm.h>

/*
 * lock from r4000 book
 */
LEAF(canlock)
	.set 	noreorder
1:
	ll	t0,0(a0)		/* a0 is argument */
	or	t1, t0, 1
	sc	t1,0(a0)
	beq	t1,zero,1b	
	nop
	j	$31			/* lock held */
	xor	v0, t0, 1

	.set 	reorder
	END(canlock)
