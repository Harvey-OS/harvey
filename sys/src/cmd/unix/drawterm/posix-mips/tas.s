#include <sys/regdef.h>
#include <sys/asm.h>

.globl tas
.ent tas 2

tas:
.set noreorder
1:
	ori	t1, zero, 12345	/* t1 = 12345 */
	ll	t0, (a0)		/* t0 = *a0 */
	sc	t1, (a0)		/* *a0 = t1 if *a0 hasn't changed; t1=success */
	beq	t1, zero, 1b		/* repeat if *a0 did change */
	nop

	j $31				/* return */
	or	v0, t0, zero		/* set return value on way out */

.set reorder
.end tas

