#include <architecture/ppc/asm_help.h>

LEAF(__tas)
	sync
	mr	r4,r3
	addi    r5,0,0x1
1:
	lwarx	r3,0,r4
	cmpwi   r3,0x0
	bne-    2f
	stwcx.	r5,0,r4
	bne-    1b		/* Lost reservation, try again */
2:
	sync
	blr
END(__tas)
