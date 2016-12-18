/* these are intended to be called from library functions ONLY.
 * at some point, we can make it all more efficient, but for now,
 * let's make it correct.
 */

#define sstatus 0x100
#define siebitmask 2
/* int _splhi() */
.globl _splhi
_splhi:
	li	a0, siebitmask
	csrrc	a0, sstatus, a0
	ret

/* int _spllo() */
.globl _spllo
_spllo:
	li	a0, siebitmask
	csrrs	a0, sstatus, a0
	ret

/* what a hack. Purely a marker for profiling.*/
.globl spldone
spldone:
	ret
