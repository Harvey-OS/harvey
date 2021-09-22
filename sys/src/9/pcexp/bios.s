/*
 * early real-mode bios calls
 */
/* we don't know what evil the bios gets up to */
#undef	BIOSCALL
#define	BIOSCALL(b)	INT $(b); CLI; CLD

#define SMAP 	0x534D4150		/* e820 signature - ASCII "SMAP" */
#define SZMAPENTRY (2*BY2V + BY2WD)	/* base, length, type */

	/*
	 * we are committed to using the bios rather than pure uefi now.
	 */
/*
 * try to enable a20 line. does nothing on old BIOSes (e.g., from 1997).
 */
	LWI(0x2401, rAX)
	BIOSCALL(0x15)

/*
 * Check for CGA mode.
 */
	LWI(0x0F00, rAX)		/* get current video mode in AL */
	BIOSCALL(0x10)
	ANDI(0x007F, rAX)
	SUBI(0x0003, rAX)		/* is it mode 3? */
	JEQ	_cgamode3

	LWI(0x0003, rAX)		/* turn on text mode 3 */
	BIOSCALL(0x10)
_cgamode3:				/* output a cheery wee message */
#ifdef PARANOID
	LWI(0x0500, rAX)		/* set active display page to 0 */
	BIOSCALL(0x10)
#endif
	CLR(rAX)
	MTSR(rAX, rDS)
	LLI(_hello(SB), rSI)
	CALL16(_cgaputs(SB))

#ifdef HAVE_FLOPPY
/*
 * Reset floppy disc system.
 * If successful, make sure the motor is off.
 */
	CLR(rAX)
	CLR(rDX)
	BIOSCALL(0x13)
	ORB(rAL, rAL)
	JNE	_floppyend
	OUTPORTB(0x3F2, 0x00)		/* turn off motor */
_floppyend:
#endif

/*
 *	do things that need to be done in real mode.
 *	the results (maximum of ~800 bytes) get written to BIOSTABLES.
 *	in a series of <4-byte-magic-number><block-of-data>
 *	the data length is dependent on the magic number.
 *
 *	this gets parsed by conf.c:/^readlsconf
 *
 *	N.B. CALL16 kills rDI, so we can't call anything.
 */
	CLR(rAX)
	SW(rAX, _ES(SB))		/* bios tables are in first 64k */
	MTSR(rAX, rES)
	LLI((BIOSTABLES-KZERO), rDI)
	SW(rDI, _DI(SB))

/*
 * APM: Check for APM1.2 BIOS support.
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

	CLD
	LWI(0x5041, rAX)		/* first 4 bytes are "APM\0" */
	STOSW
	LWI(0x004D, rAX)
	STOSW

	LWI(8, rCX)			/* pop the saved APM data */
_apmpop:
	POPR(rAX)
	STOSW				/* *(ushort *)(ES:DI++) = AL */
	LOOP	_apmpop
_apmend:
	CLR(rAX)			/* write APM terminator */
	STOSW
	STOSW

/*
 * Try to retrieve the 0xE820 memory map, available since 2002 at the latest.
 * This is weird because some BIOS do not seem to preserve
 * ES/DI on failure. Consequently they may not be valid after the E820 loop.
 */
	/*
	 * start at the beginning.  zero all 32 bits of BX because some
	 * BIOSes care.
	 */
	OPSIZE; XORL BX, BX
	PUSHR(rBX)			/* keep loop count on stack */
					/* BX is the continuation value */
	MFSR(rES, rAX)			/* save es & di */
	SW(rAX, _ES(SB))
	SW(rDI, _DI(SB))		/* save DI */
_e820loop:
	POPR(rAX)
	INC(rAX)
	PUSHR(rAX)			/* doesn't affect FLAGS */
	CMPI(Maxmmap, rAX)
	JGT	_e820pop

	LLI(SZMAPENTRY, rCX)		/* buffer size */
	LLI(SMAP, rDX)			/* signature - ASCII "SMAP" */
	LLI(0x0000E820, rAX)		/* function code */
	BIOSCALL(0x15)			/* writes 20 bytes at (es,di) */
	JCS	_e820pop		/* some kind of error */

	LLI(SMAP, rDX)
	CMPL(rDX, rAX)			/* verify correct BIOS version */
	JNE	_e820pop
	LLI(SZMAPENTRY, rDX)
	CMPL(rDX, rCX)			/* verify correct count */
	JNE	_e820pop

	SUBI(BY2WD, rDI)		/* overwrite last terminator */
	CLD
	LWI(0x414D, rAX)		/* first 4 bytes are "MAP\0" */
	STOSW
	LWI(0x0050, rAX)
	STOSW

	ADDI(SZMAPENTRY, rDI)		/* skip this memory map entry */

	CLR(rAX)			/* write terminator */
	STOSW
	STOSW
	SW(rDI, _DI(SB))		/* save DI */

	OPSIZE; ORL BX, BX		/* 32-bit zero if last entry */
	/*
	 * conditional jumps only take byte displacements, so for targets
	 * farther away, we must use an unconditional long jump.
	 */
	JEQ	_e820pop
	JMP	_e820loop
_e820pop:
	POPR(rAX)			/* loop count */
	LW(_DI(SB), rDI)		/* reload value contents */
	CLR(rAX)
	MTSR(rAX, rES)
	JMP	realdone

/* output a message at DS:SI in real mode by calling the BIOS. */
TEXT _cgaputs(SB), $0
	OPSIZE; PUSHAL
_cgaputsloop:
	CLD
	CLR(rBX)		/* page number */
	LODSB			/* AL = *(uchar *)(DS:SI++) */
	ORB(rAL, rAL)
	JEQ	_cgaend

	LBI(0x0E,rAH)
	BIOSCALL(0x10)
	JMP	_cgaputsloop
_cgaend:
	OPSIZE; POPAL
	RET

realdone:
