#include <atom.h>

/*
 * Macros for accessing page table entries; change the
 * C-style array-index macros into a page table byte offset
 * RV64 PTEs are vlongs, so shift by 3 bits.
 */
// #define PTROOTO(v)	((PTLX((v), 3))<<3)
#define PDPO(v)		((PTLX((v), 2))<<3)
#define PDO(v)		((PTLX((v), 1))<<3)
#define PTO(v)		((PTLX((v), 0))<<3)

#ifndef MASK
#define MASK(w)		((1u  <<(w)) - 1)
#define VMASK(w)	((1ull<<(w)) - 1)
#endif

/* instructions not yet known to ia and ja */
#define ECALL	SYS	$0
#define EBREAK	SYS	$1
#define URET	SYS	$2
#define SRET	SYS	$0x102
#define WFI	SYS	$0x105
#define MRET	SYS	$0x302

/*
 * strap and mtrap macros
 */
#define PUSH(n)		MOV R(n), ((n)*XLEN)(R2)
#define POP(n)		MOV ((n)*XLEN)(R2), R(n)

#define SAVER1_31 \
	PUSH(1); PUSH(2); PUSH(3); PUSH(4); PUSH(5); PUSH(6); PUSH(7); PUSH(8);\
	PUSH(9); PUSH(10); PUSH(11); PUSH(12); PUSH(13); PUSH(14); PUSH(15);\
	PUSH(16); PUSH(17); PUSH(18); PUSH(19); PUSH(20); PUSH(21); PUSH(22);\
	PUSH(23); PUSH(24); PUSH(25); PUSH(26); PUSH(27); PUSH(28); PUSH(29);\
	PUSH(30); PUSH(31)

#define POPR8_31 \
	POP(8); POP(9); POP(10); POP(11); POP(12); POP(13); POP(14); POP(15);\
	POP(16); POP(17); POP(18); POP(19); POP(20); POP(21); POP(22); POP(23);\
	POP(24); POP(25); POP(26); POP(27); POP(28); POP(29); POP(30); POP(31)

#undef	PTLX
#define PTLX(v, l) (((v) >> ((l)*PTSHFT+PGSHFT)) & ((1<<PTSHFT)-1))
