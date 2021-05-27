#include "x16.h"

#undef ORB

TEXT a20test(SB), $0
	LONG $1234567

TEXT a20check(SB), $0
	MOVL $10000, CX
_loop:
	LEAL a20test(SB), AX
	MOVL (AX), BX
	ADDL $12345, BX
	MOVL BX, (AX)
	ORL $(1<<20), AX
	MOVL (AX), AX
	CMPL AX, BX
	JNZ _done
	LOOP _loop
	RET
_done:
	/* return directly to caller of a20() */
	ADDL $4, SP
	XORL AX, AX
	RET

TEXT a20(SB), $0
	CALL a20check(SB)

	/* try bios */
	CALL rmode16(SB)
	STI
	LWI(0x2401, rAX)
	BIOSCALL(0x15)
	CALL16(pmode32(SB))

	CALL a20check(SB)

	/* try fast a20 */
	MOVL $0x92, DX
	INB
	TESTB $2, AL
	JNZ _no92
	ORB $2, AL
	ANDB $0xfe, AL
	OUTB
_no92:
	CALL a20check(SB)

	/* try keyboard */
	CALL kbdempty(SB)
	MOVL $0x64, DX
	MOVB $0xd1, AL	/* command write */
	OUTB
	CALL kbdempty(SB)
	MOVL $0x60, DX
	MOVB $0xdf, AL	/* a20 on */
	OUTB
	CALL kbdempty(SB)
	MOVL $0x64, DX
	MOVB $0xff, AL	/* magic */
	OUTB
	CALL kbdempty(SB)

	CALL a20check(SB)

	/* fail */
	XORL AX, AX
	DECL AX
	RET

TEXT kbdempty(SB), $0
_kbdwait:
	MOVL $0x64, DX
	INB
	TESTB $1, AL
	JZ _kbdempty
	MOVL $0x60, DX
	INB
	JMP _kbdwait
_kbdempty:
	TESTB $2, AL
	JNZ _kbdwait
	RET
