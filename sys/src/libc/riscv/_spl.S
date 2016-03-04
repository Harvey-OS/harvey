/* these are intended to be called from library functions ONLY.
 * at some point, we can make it all more efficient, but for now,
 * let's make it correct.
 */

/* int _splhi() */
.globl _splhi
_splhi:	
	li	a0, 0
	csrrc	a0, 0x100, a0
	ret

/* int _spllo() */
.globl _spllo
_spllo:
	li	a0, 0
	csrrs	a0, 0x100, a0
	ret
