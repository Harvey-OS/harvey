.text

.globl ainc				/* int32_t ainc(int32_t *); */
ainc:
	li		a1, 1
	amoadd.w.aq	a0, a1, 0(a0)
	ret
.globl adec				/* int32_t adec(int32_t *); */
adec:
	li		a1, -1
	amoadd.w.aq	a0, a1, 0(a0)
	ret
.globl _tas				/* int _tas(int *); */
_tas:
	li           a1, 1
	amoswap.w.aq a0, a1, 0(a0)
	ret

