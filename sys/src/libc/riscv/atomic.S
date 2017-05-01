.text

.globl ainc				/* int32_t ainc(int32_t *); */
ainc:
	li		t0, 1
	amoadd.w.aq	t1, t0, 0(a0)
	add		a0, t1, t0
	ret

.globl adec				/* int32_t adec(int32_t *); */
adec:
	li		t0, -1
	amoadd.w.aq	t1, t0, 0(a0)
	add		a0, t1, t0
	ret

/* This works fine either way as we are little endian. */
/* boy we have a lot of names for this. I don't know how *that* happened. */
.globl _tas32				/* int _tas32(int *); */
_tas32:
.globl tas32				/* int _tas32(int *); */
tas32:
.globl _tas				/* int _tas(int *); */
_tas:
	li t0, 1
	amoswap.w.aq t0, t0, 0(a0)
	bnez t0, 1f
	li a0, 0
	ret
/* failure. */
1:
	li a0, 1
	ret

.globl aswap				/* int aswap(int *, int); */
aswap:
	amoswap.w.aq a0, a1, 0(a0)
	ret

.globl cas32
// int	cas32(void* %rdi, uint32_t %esi, uint32_t %edx);
// a0 holds address of memory location
// a1 holds expected value (old)
// a2 holds desired value (new)
// v0 return value, 1 if successful, 0 otherwise
cas32:
	lr.w t0, (a0)  // Load original value
	bne t0, a1, 1f # Doesn’t match, so fail
	sc.w a0, a2, (a0) # Try to update
	bne zero, a0, 1f
	li a0, 1
	jr ra
1:
	li a0, 0 # Preset return to fail
	jr ra

.globl mfence
mfence:
	fence
	ret
