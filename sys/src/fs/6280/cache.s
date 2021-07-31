#include "mem.h"

#define NOOP	WORD	$0x0

#define FLUSH	MOVWR
#define INVAL	MOVWR
#define LCACHE	MOVWL
#define SCACHE	MOVWL

/*
 * invalidate the virtual data cache.
 * cache line-size is 8 bytes.
 * the data cache is selected if the LSB
 * of the index in the INVAL is 1.
 */
TEXT invaldline(SB), $-4		/* invald16(va=R1) */

	MOVW	M(STATUS), R22
	MOVW	$MM, R15
	MOVW	R15, M(STATUS)
	NOOP;NOOP;NOOP

	INVAL	R0, 0x01(R1)		/* invalidate 16 lines */
	INVAL	R0, 0x09(R1)
	INVAL	R0, 0x11(R1)
	INVAL	R0, 0x19(R1)
	INVAL	R0, 0x21(R1)
	INVAL	R0, 0x29(R1)
	INVAL	R0, 0x31(R1)
	INVAL	R0, 0x39(R1)
	INVAL	R0, 0x41(R1)
	INVAL	R0, 0x49(R1)
	INVAL	R0, 0x51(R1)
	INVAL	R0, 0x59(R1)
	INVAL	R0, 0x61(R1)
	INVAL	R0, 0x69(R1)
	INVAL	R0, 0x71(R1)
	INVAL	R0, 0x79(R1)

	MOVW	R22, M(STATUS)
	RET

TEXT wback2cache(SB), $-4
	MOVW	R1, 0(FP)		/* addr */
	MOVW	M(STATUS), R22		/* save status */
	MOVW	$MM, R15		/* disable interrupts */
	MOVW	R15, M(STATUS)		/*  and enter MM mode */

	MOVW	4(FP), R5		/* nbytes */
	MOVW	$0x80, R8		/* line size */

	SGTU	R8, R5, R12		/* make nbytes at least line size */
	BEQ	R12, _wback20
	MOVW	R8, R5

_wback20:
	MOVW	0(FP), R9		/* addr */
	SLL	$3, R9, R11		/* shift off K0/K1 bits */
	SRL	$0x11, R11		/*  and back to get true pfn */

	MOVW	$0xDFFFFFFF, R12	/* tentative mask for FLUSH address */
	MOVW	$0x0F, R15		/* pfn mask */
	ADD	R5, R9, R10		/* addr+1 of last line for FLUSH */
	AND	R15, R11, R14		/* low order 4 bits of pfn will be 12:9 */
	BNE	R15, R14, _wback21	/*  except 0x0F gets mapped to 0x07 */

	MOVW	$0x07, R14
	MOVW	$0xDFFDFFFF, R12	/* correct FLUSH address mask */

_wback21:
	OR	$0x01F0, R14, R8	/* ptags bits will be bits 17:13 */
	SLL	$9, R8			/*  and shift into proper position */
	SRL	$7, R9			/* start to align FLUSH address */
	AND	$0x7F, R9, R14		/* isolate cache line index */
	SLL	$2, R14			/*  and shift into proper position */
	OR	R14, R8			/*  and merge to form complete address */

	SLL	$7, R9			/* align FLUSH address on line boundary */
	SUBU	$0x80, R10		/* set up ending address for loop control */
	AND	R12, R9			/* adjust address to avoid using upper */
	AND	R12, R10		/*   1/16 of cache, and convert to K0 */

	MOVW	$WBACK2LOOP, R14	/* address of loop in low memory to jump to */
	MOVW	$0x3FFFC, R15		/* benign scache address */
	JMP	(R14)

TEXT _wback2loop(SB), $-4
_wback2l:
	LCACHE	(R8), R12		/* side-0 ptag */
	NOOP
	LCACHE	1(R8), R13		/* side-1 ptag */
	SRL	$10, R12		/* r-justify side-0 pfn */
	SRL	$10, R13		/* r-justify side-1 pfn */
	BNE	R12, R11, _wback2l0	/* matches side-0? */

	FLUSH	(R9), R0		/* match - flush side-0 */
	SCACHE	R0, (R15)		/* same i-cache line as FLUSH */

_wback2l0:
	ADD	$4, R8			/* address of next ptag */
	BNE	R13, R11, _wback2l1	/* matches side-1? */

	FLUSH	1(R9), R0		/* match - flush side-1 */
	SCACHE	R0, (R15)		/* same i-cache line as FLUSH */

_wback2l1:
	SGT	R10, R9, R12		/* all done? */
	ADD	$0x80, R9		/* address for next flush */
	BNE	R12, _wback2l

	MOVW	R22, M(STATUS)		/* restore status */
	RET

TEXT inval2cache(SB), $-4
	MOVW	R1, 0(FP)		/* addr */
	MOVW	M(STATUS), R22		/* save status */
	MOVW	$MM, R15		/* disable interrupts */
	MOVW	R15, M(STATUS)		/*  and enter MM mode */

	MOVW	4(FP), R5		/* nbytes */
	MOVW	$0x80, R8		/* line size */

	SGT	R8, R5, R12		/* make nbytes at least line size */
	BEQ	R12, _inval20
	MOVW	R8, R5

_inval20:
	MOVW	0(FP), R9		/* addr */
	SLL	$3, R9, R11		/* shift off K0/K1 bits */
	SRL	$0x11, R11		/*  and back to get true pfn */

	MOVW	$0xDFFFFFFF, R12	/* tentative mask for FLUSH address */
	MOVW	$0x0F, R15		/* pfn mask */
	ADD	R5, R9, R10		/* addr+1 of last line for FLUSH */
	AND	R15, R11, R14		/* low order 4 bits of pfn will be 12:9 */
	BNE	R15, R14, _inval21	/*  except 0x0F gets mapped to 0x07 */

	MOVW	$0x07, R14
	MOVW	$0xDFFDFFFF, R12	/* correct FLUSH address mask */

_inval21:
	OR	$0x01F0, R14, R8	/* ptags bits will be bits 17:13 */
	SLL	$9, R8			/*  and shift into proper position */
	SRL	$7, R9			/* start to align FLUSH address */
	AND	$0x7F, R9, R14		/* isolate cache line index */
	SLL	$2, R14			/*  and shift into proper position */
	OR	R14, R8			/*  and merge to form complete address */

	SLL	$7, R9			/* align FLUSH address on line boundary */
	AND	R12, R9			/* adjust address to avoid using upper */
	AND	R12, R10		/*   1/16 of cache, and convert to K0 */
	MOVW	$0xFFFFFC00, R15	/* invalid ptag value */

	MOVW	$INVAL2LOOP, R14	/* address of loop in low memory to jump to */
	JMP	(R14)

TEXT _inval2loop(SB), $-4
_inval2l:
	LCACHE	(R8), R12		/* side-0 ptag */
	NOOP
	LCACHE	1(R8), R13		/* side-1 ptag */
	SRL	$10, R12		/* r-justify side-0 pfn */
	SRL	$10, R13		/* r-justify side-1 pfn */
	BNE	R12, R11, _inval2l0	/* matches side-0? */

	NOOP				/* align FLUSH/SCACHE to live in the
	NOOP				/*  same i-cache line */
	NOOP				/*  (why does one of these disappear, ken?) */
	FLUSH	(R9), R0		/* match - flush side-0 */
	INVAL	R0, (R9)		/*  and invalidate */
	SCACHE	R15, (R8)		/*  and clobber ptag */

_inval2l0:
	BNE	R13, R11, _inval2l1	/* matches side-1? */

	FLUSH	1(R9), R0		/* match - flush side-1 */
	INVAL	R0, 1(R9)		/*  and invalidate */
	SCACHE	R15, 1(R8)		/*  and clobber ptag */

_inval2l1:
	ADD	$0x80, R9		/* address for next flush */
	ADD	$4, R8			/* address of next ptag */
	SGT	R10, R9, R12		/* all done? */
	BNE	R12, _inval2l

	MOVW	R22, M(STATUS)		/* restore status */
	RET

TEXT cacheflush(SB), $-4
	MOVW	M(STATUS), R22
	MOVW	$MM, R15
	MOVW	R15, M(STATUS)

	MOVW	$(16*1024), R9		/* d-cache size */
	MOVW	$0, R8			/* initial starting address */
	SUBU	$0x80, R9		/*  and ending address */

_cf0:
	INVAL	R0, 0x01(R8)		/* do 16 lines at a time */
	INVAL	R0, 0x09(R8)
	INVAL	R0, 0x11(R8)
	INVAL	R0, 0x19(R8)
	INVAL	R0, 0x21(R8)
	INVAL	R0, 0x29(R8)
	INVAL	R0, 0x31(R8)
	INVAL	R0, 0x39(R8)
	INVAL	R0, 0x41(R8)
	INVAL	R0, 0x49(R8)
	INVAL	R0, 0x51(R8)
	INVAL	R0, 0x59(R8)
	INVAL	R0, 0x61(R8)
	INVAL	R0, 0x69(R8)
	INVAL	R0, 0x71(R8)
	INVAL	R0, 0x79(R8)
	MOVW	R8, R7
	ADD	$0x80, R8
	BNE	R7, R9, _cf0

	MOVW	$(64*1024), R9		/* i-cache size */
	MOVW	$0, R8			/* initial starting address */
	SUBU	$0x200, R9		/*  and ending address */

_cf1:
	INVAL	R0, 0x000(R8)		/* do 16 lines at a time */
	INVAL	R0, 0x020(R8)
	INVAL	R0, 0x040(R8)
	INVAL	R0, 0x060(R8)
	INVAL	R0, 0x080(R8)
	INVAL	R0, 0x0A0(R8)
	INVAL	R0, 0x0C0(R8)
	INVAL	R0, 0x0E0(R8)
	INVAL	R0, 0x100(R8)
	INVAL	R0, 0x120(R8)
	INVAL	R0, 0x140(R8)
	INVAL	R0, 0x160(R8)
	INVAL	R0, 0x180(R8)
	INVAL	R0, 0x1A0(R8)
	INVAL	R0, 0x1C0(R8)
	INVAL	R0, 0x1E0(R8)
	MOVW	R8, R7
	ADD	$0x200, R8
	BNE	R7, R9, _cf1

	MOVW	$0x3E000, R8		/* start address for ptags */
	MOVW	$KZERO, R9		/* start address for INVAL/FLUSH */
	MOVW	$0x3C000, R1		/*  and end address */
	ADD	R1, R9, R10
	MOVW	$0xFFFFFC00, R15	/* invalid ptag value */

	MOVW	$CACHEFLOOP, R14	/* address of loop in low memory to jump to */
	JMP	(R14)

TEXT _cachefloop(SB), $-4
_cachefl:
	FLUSH	(R9), R0		/* flush side-0 */
	INVAL	R0, (R9)		/*  and invalidate */
	SCACHE	R15, (R8)		/*  and clobber ptag */
	FLUSH	0x01(R9), R0		/* flush side-1 */
	INVAL	R0, 0x01(R9)		/*  and invalidate */
	SCACHE	R15, 0x01(R8)		/*  and clobber ptag */
	ADD	$0x80, R9		/* address of next FLUSH/INVAL */
	ADD	$4, R8			/* address of next ptag */
	BNE	R9, R10, _cachefl

	MOVW	R22, M(STATUS)
	RET
