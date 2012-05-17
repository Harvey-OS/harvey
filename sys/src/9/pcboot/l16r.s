/*
 * Protected-mode bootstrap, to be jumped to by a Primary Bootstrap Sector.
 * Load with -H3 -R4 -T0xNNNNNNNN to get a binary image with no header.
 * Note that the processor is in 'real' mode on entry, so care must be taken
 * as the assembler assumes protected mode, hence the sometimes weird-looking
 * code to assure 16-bit instructions are issued.
 */
#include "mem.h"
#include "/sys/src/boot/pc/x16.h"

#undef BIOSCALL		/* we don't know what evil the bios gets up to */
#define BIOSCALL(b)	INT $(b); CLI

#define CMPL(r0, r1)	BYTE $0x66; CMP(r0, r1)
#define LLI(i, rX)	BYTE $0x66;		/* i -> rX */		\
			BYTE $(0xB8+rX);				\
			LONG $i;

/*
 * Start:
 *	disable interrupts;
 *	set all segments;
 *	create temporary stack.
 */
TEXT _start16r(SB), $0
	CLI				/* interrupts off */

	MFSR(rCS, rAX)
	MTSR(rAX, rDS)			/* set the data segment */

	LWI(0, rAX)			/* always put stack in first 64k */
	MTSR(rAX, rSS)
	LWI(PXEBASE, rSP)		/* set the stack */

	LWI(0x2401, rAX)		/* enable a20 line */
	BIOSCALL(0x15)

/*
 * Do any real-mode BIOS calls before going to protected mode.
 * Data will be stored at BIOSTABLES, not ROUND(end, BY2PG) as before.
 */

/*
 * Check for CGA mode.
 */
_cgastart:
	LWI(0x0F00, rAX)		/* get current video mode in AL */
	BIOSCALL(0x10)
	ANDI(0x007F, rAX)
	SUBI(0x0003, rAX)		/* is it mode 3? */
	JEQ	_cgaputs

	LWI(0x0003, rAX)		/* turn on text mode 3 */
	BIOSCALL(0x10)

_cgaputs:				/* output a cheery wee message */
	LWI(_hello(SB), rSI)
	CLR(rBX)
_cgaputsloop:
	LODSB
	ORB(rAL, rAL)
	JEQ	_cgaend

	LBI(0x0E,rAH)
	BIOSCALL(0x10)
	JMP	_cgaputsloop
_cgaend:

/*
 * Reset floppy disc system.
 * If successful, make sure the motor is off.
 */
_floppystart:
	CLR(rAX)
	CLR(rDX)
	BIOSCALL(0x13)
	ORB(rAL, rAL)
	JNE	_floppyend
	OUTPORTB(0x3F2, 0x00)		/* turn off motor */
_floppyend:

	LLI(BIOSTABLES, rAX)	/* tables in low memory, not after end */
	OPSIZE; ANDL $~(BY2PG-1), AX
	OPSIZE; SHRL $4, AX
	SW(rAX, _ES(SB))
	CLR(rDI)
	SW(rDI, _DI(SB))

	MTSR(rAX, rES)
	
/*
 * Check for APM1.2 BIOS support.
 */
	LWI(0x5304, rAX)		/* disconnect anyone else */
	CLR(rBX)
	BIOSCALL(0x15)
	JCS	_apmfail

	LWI(0x5303, rAX)		/* connect */
	CLR(rBX)
	CLC
	BIOSCALL(0x15)
	JCC	_apmpush
_apmfail:
	LW(_ES(SB), rAX)		/* no support */
	MTSR(rAX, rES)
	LW(_DI(SB), rDI)
	JCS	_apmend

_apmpush:
	OPSIZE; PUSHR(rSI)		/* save returned APM data on stack */
	OPSIZE; PUSHR(rBX)
	PUSHR(rDI)
	PUSHR(rDX)
	PUSHR(rCX)
	PUSHR(rAX)

	LW(_ES(SB), rAX)
	MTSR(rAX, rES)
	LW(_DI(SB), rDI)

	LWI(0x5041, rAX)		/* first 4 bytes are APM\0 */
	STOSW
	LWI(0x004D, rAX)
	STOSW

	LWI(8, rCX)			/* pop the saved APM data */
_apmpop:
	POPR(rAX)
	STOSW
	LOOP	_apmpop
_apmend:

/*
 * Try to retrieve the 0xE820 memory map.
 * This is weird because some BIOS do not seem to preserve
 * ES/DI on failure. Consequently they may not be valid
 * at _e820end:.
 */
	SW(rDI, _DI(SB))		/* save DI */
	CLR(rAX)			/* write terminator */
	STOSW
	STOSW

	CLR(rBX)
	PUSHR(rBX)			/* keep loop count on stack */
					/* BX is the continuation value */
_e820loop:
	POPR(rAX)
	INC(rAX)
	PUSHR(rAX)			/* doesn't affect FLAGS */
	CMPI(32, rAX)			/* mmap[32+1] in C code */
	JGT	_e820pop

	LLI(20, rCX)			/* buffer size */
	LLI(0x534D4150, rDX)		/* signature - ASCII "SMAP" */
	LLI(0x0000E820, rAX)		/* function code */

	BIOSCALL(0x15)			/* writes 20 bytes at (es,di) */

	JCS	_e820pop		/* some kind of error */
	LLI(0x534D4150, rDX)
	CMPL(rDX, rAX)			/* verify correct BIOS version */
	JNE	_e820pop
	LLI(20, rDX)
	CMPL(rDX, rCX)			/* verify correct count */
	JNE	_e820pop

	SUBI(4, rDI)			/* overwrite terminator */
	LWI(0x414D, rAX)		/* first 4 bytes are "MAP\0" */
	STOSW
	LWI(0x0050, rAX)
	STOSW

	ADDI(20, rDI)			/* bump to next entry */

	SW(rDI, _DI(SB))		/* save DI */
	CLR(rAX)			/* write terminator */
	STOSW
	STOSW

	OR(rBX, rBX)			/* zero if last entry */
	JNE	_e820loop

_e820pop:
	POPR(rAX)			/* loop count */
	LW(_DI(SB), rDI)
	CLR(rAX)
	MTSR(rAX, rES)
_e820end:

/*
 * Done with BIOS calls for now.  realmode calls may use the BIOS later.
 */

/*
 * Load a basic GDT to map 4GB, turn on the protected mode bit in CR0,
 * set all the segments to point to the new GDT then jump to the 32-bit code.
 */
_real:
	LGDT(_gdtptr16r(SB))		/* load a basic gdt */

	MFCR(rCR0, rAX)
	ORI(1, rAX)
	MTCR(rAX, rCR0)			/* turn on protected mode */
	DELAY				/* JMP .+2 */

	LWI(SELECTOR(1, SELGDT, 0), rAX)/* set all segments */
	MTSR(rAX, rDS)
	MTSR(rAX, rES)
	MTSR(rAX, rFS)
	MTSR(rAX, rGS)
	MTSR(rAX, rSS)

	FARJUMP32(SELECTOR(2, SELGDT, 0), _start32p-KZERO(SB))

TEXT _hello(SB), $0
	BYTE $'\r';
	BYTE $'\n';
	BYTE $'P'; BYTE $'l'; BYTE $'a'; BYTE $'n';
	BYTE $' '; BYTE $'9'; BYTE $' '; BYTE $'f';
	BYTE $'r'; BYTE $'o'; BYTE $'m'; BYTE $' ';
	BYTE $'B'; BYTE $'e'; BYTE $'l'; BYTE $'l';
	BYTE $' '; BYTE $'L'; BYTE $'a'; BYTE $'b';
	BYTE $'s'; 
	BYTE $'\z';

TEXT _DI(SB), $0
	LONG $0

TEXT _ES(SB), $0
	LONG $0
