/*
 * Macros for accessing page table entries; change the
 * C-style array-index macros into a page table byte offset
 */
#define PML4O(v)	((PTLX((v), 3))<<3)
#define PDPO(v)		((PTLX((v), 2))<<3)
#define PDO(v)		((PTLX((v), 1))<<3)
#define PTO(v)		((PTLX((v), 0))<<3)

/* trashes AX, DX */
// #define DBGPUT(c) MOVL $0x3F8, DX; MOVL $(c), AX; OUTB
#define DBGPUT(c)
