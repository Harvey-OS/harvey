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

/* This works fine either way as we are little endian. */
/* boy we have a lot of names for this. I don't know how *that* happened. */
.globl _tas32				/* int _tas32(int *); */
_tas32:
.globl tas32				/* int _tas32(int *); */
tas32:
.globl _tas				/* int _tas(int *); */
_tas:
	li           a1, 1
	amoswap.w.aq a0, a1, 0(a0)
	ret

.globl aswap				/* int aswap(int *, int); */
aswap:
	amoswap.w.aq a0, a1, 0(a0)
	ret

.globl cas32
// int	cas32(void* %rdi, uint32_t %esi, uint32_t %edx);
// a0 holds address of memory location
// a1 holds expected value
// a2 holds desired value
// v0 return value, 0 if successful, !0 otherwise
cas32:
	lr.w x16, (a0)  // Load original value
	li x16, 1 # Preset return to fail
	bne x17, a1, return # Doesn’t match, so fail
	sc.w x16, a2, (a0) # Try to update
return: jr ra

.globl mfence
mfence:
	fence
	ret
