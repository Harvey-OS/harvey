#include "x16.h"
#include "mem.h"

TEXT apm(SB), $0
	MOVL id+4(SP), BX
	CALL rmode16(SB)

	PUSHR(rBX)
	LWI(0x5300, rAX)
	BIOSCALL(0x15)
	POPR(rBX)
	JC noapm

	PUSHR(rBX)
	LWI(0x5304, rAX)
	BIOSCALL(0x15)
	POPR(rBX)
	CLC

	/* connect */
	LWI(0x5303, rAX)
	BIOSCALL(0x15)
	JC noapm

	OPSIZE; PUSHR(rSI)
	OPSIZE; PUSHR(rBX)
	PUSHR(rDI)
	PUSHR(rDX)
	PUSHR(rCX)
	PUSHR(rAX)

	LWI(CONFADDR, rDI)

	/*
	 * write APM data.  first four bytes are APM\0.
	 */
	LWI(0x5041, rAX)
	STOSW

	LWI(0x004d, rAX)
	STOSW

	LWI(8, rCX)
apmmove:
	POPR(rAX)
	STOSW
	LOOP apmmove

noapm:
	CALL16(pmode32(SB))
	RET
