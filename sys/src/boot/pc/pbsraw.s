/*
 *  Partition Boot Sector. Loaded at 0x7C00:
 *	8a pbsraw.s; 8l -o pbsraw -l -H3 -T0x7C00 pbsraw.8
 * Will load the target at LOADSEG*16+LOADOFF, so the target
 * should be probably be loaded with LOADOFF added to the
 * -Taddress.
 * If LOADSEG is a multiple of 64KB and LOADOFF is 0 then
 * targets larger than 64KB can be loaded.
 *
 * This code is uses Enhanced BIOS Services for Disc Drives and
 * can be used with discs up to 137GB in capacity.
 *
 * It relies on the _startlba,  _filesz and _sectsz containing the start lba of
 * the loader and filesz to contain the size of the file and the sector size.
 * The sector size can be probably detected by the bios.
 */
#include "x16.h"

#define LOADSEG		(0x10000/16)	/* where to load code (64KB) */
#define LOADOFF		0

/*
 * Data is kept on the stack, indexed by rBP.
 */
#define Xdap		0x00		/* disc address packet */
#define Xrootsz		0x10		/* file data area */
#define Xdrive		0x12		/* boot drive, passed by BIOS or MBR */
#define Xtotal		0x14		/* sum of allocated data above */

TEXT _magic(SB), $0
	BYTE $0xEB; BYTE $0x3C;		/* jmp .+ 0x3C  (_start0x3E) */
	BYTE $0x90			/* nop */
TEXT _startlba(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
TEXT _filesz(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
TEXT _sectsz(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
TEXT _pad(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00

_start0x3E:
	CLI
	CLR(rAX)
	MTSR(rAX, rSS)			/* 0000 -> rSS */
	MTSR(rAX, rDS)			/* 0000 -> rDS, source segment */
	MTSR(rAX, rES)
	LWI(_magic-Xtotal(SB), rSP)
	MW(rSP, rBP)			/* set the indexed-data pointer */

	SBPB(rDL, Xdrive)		/* save the boot drive */

	/* booting from a CD starts us at 7C0:0.  Move to 0:7C00 */
	PUSHR(rAX)
	LWI(_nxt(SB), rAX)
	PUSHR(rAX)
	BYTE $0xCB			/* FAR RET */

TEXT _nxt(SB), $0
	STI

	LWI(confidence(SB), rSI)	/* for that warm, fuzzy feeling */
	CALL16(BIOSputs(SB))

	LBI(0x41, rAH)			/* check extensions present */
	LWI(0x55AA, rBX)
	LXB(Xdrive, xBP, rDL)		/* drive */
	BIOSCALL(0x13)			/* CF set on failure */
	JCS _jmp00
	CMPI(0xAA55, rBX)
	JNE _jmp00
	ANDI(0x0001, rCX)
	JNE _jmp01

_jmp00:
	CALL16(buggery(SB))

_jmp01:
	SBPWI(0x0010, Xdap+0)		/* reserved + packet size */
	SBPW(rCX, Xdap+2)		/* reserved + # of blocks to transfer */

	DEC(rCX)
	SBPW(rCX, Xdap+12)
	SBPW(rCX, Xdap+14)

	CALL16(dreset(SB))

_jmp02:
	CLR(rBX)			/* a handy value */

	LW(_startlba(SB), rAX)
	LW(_startlba+2(SB), rDX)
	CALL16(printDXAX(SB))
	PUSHR(rAX)
	PUSHR(rDX)
	LW(_filesz(SB), rAX)
	LW(_filesz+2(SB), rDX)
	CALL16(printDXAX(SB))

	MW(rAX, rCX)
	POPR(rDX)
	POPR(rAX)

	LWI(LOADSEG, rBX)		/* address to load into (seg+offset) */
	MTSR(rBX, rES)			/*  seg */
	LWI(LOADOFF, rBX)		/*  offset */

_readboot:
	CALL16(BIOSread(SB))		/* read the sector */

	LW(_sectsz(SB), rDI)		/* bump addresses/counts */
	ADD(rDI, rBX)
	JCC _incsecno

	MFSR(rES, rDI)			/* next 64KB segment */
	ADDI(0x1000, rDI)
	MTSR(rDI, rES)

_incsecno:
	CLR(rDI)
	INC(rAX)
	ADC(rDI, rDX)
	LOOP _readboot

	LWI(LOADSEG, rDI)		/* set rDS for loaded code */
	MTSR(rDI, rDS)
	FARJUMP16(LOADSEG, LOADOFF)	/* no deposit, no return */

TEXT buggery(SB), $0
	LWI(error(SB), rSI)
	CALL16(BIOSputs(SB))

_wait:
	CLR(rAX)			/* wait for almost any key */
	BIOSCALL(0x16)

_reset:
	CLR(rBX)			/* set ES segment for BIOS area */
	MTSR(rBX, rES)

	LWI(0x0472, rBX)		/* warm-start code address */
	LWI(0x1234, rAX)		/* warm-start code */
	POKEW				/* MOVW	AX, ES:[BX] */

	FARJUMP16(0xFFFF, 0x0000)	/* reset */

/*
 * Read a sector from a disc. On entry:
 *   rDX:rAX	sector number
 *   rES:rBX	buffer address
 */
TEXT BIOSread(SB), $0
	LWI(5, rDI)			/* retry count (ATAPI ZIPs suck) */
_retry:
	PUSHA				/* may be trashed by BIOSCALL */

	SBPW(rBX, Xdap+4)		/* transfer buffer :offset */
	MFSR(rES, rDI)			/* transfer buffer seg: */
	SBPW(rDI, Xdap+6)
	SBPW(rAX, Xdap+8)		/* LBA (64-bits) */
	SBPW(rDX, Xdap+10)

	MW(rBP, rSI)			/* disk address packet */
	LBI(0x42, rAH)			/* extended read */
	LBPB(Xdrive, rDL)		/* form drive */
	BIOSCALL(0x13)			/* CF set on failure */
	JCC _BIOSreadret

	POPA
	DEC(rDI)			/* too many retries? */
	JEQ _ioerror

	CALL16(dreset(SB))
	JMP _retry

_ioerror:
	LWI(ioerror(SB), rSI)
	CALL16(BIOSputs(SB))
	JMP _wait

_BIOSreadret:
	POPA
	RET

TEXT dreset(SB), $0
	PUSHA
	CLR(rAX)			/* rAH == 0 == reset disc system */
	LBPB(Xdrive, rDL)
	BIOSCALL(0x13)
	ORB(rAH, rAH)			/* status (0 == success) */
	POPA
	JNE _ioerror
	RET

TEXT printsharp(SB), $0
	LWI(sharp(SB), rSI)
_doprint:
	CALL16(BIOSputs(SB))
	RET

TEXT printspace(SB), $0
	LWI(space(SB), rSI)
	JMP _doprint

TEXT printnl(SB), $0
	LWI(nl(SB), rSI)
	JMP _doprint

/*
 * Output a string to the display.
 * String argument is in rSI.
 */
TEXT BIOSputs(SB), $0
	PUSHA
	CLR(rBX)
_BIOSputs:
	LODSB
	ORB(rAL, rAL)
	JEQ _BIOSputsret

	LBI(0x0E, rAH)
	BIOSCALL(0x10)
	JMP _BIOSputs

_BIOSputsret:
	POPA
	RET

/*
 * Output a register to the display.
 */
TEXT printAX(SB), $0
	PUSHR(rAX)
	PUSHR(rBX)
	PUSHR(rCX)
	PUSHR(rDI)

	LWI(4, rCX)
	LWI(numbuf+4(SB), rSI)

_nextchar:
	DEC(rSI)
	MW(rAX, rBX)
	ANDI(0x000F, rBX)
	ADDI(0x30, rBX)	/* 0x30 = '0' */
	CMPI(0x39, rBX)	/* 0x39 = '9' */
	JLE _dowrite
	ADDI(0x07, rBX)	/* 0x07 = 'A'-(1+'9')*/

_dowrite:
	SXB(rBL, 0, xSI)
	SHRI(4, rAX)

	DEC(rCX)
	JNE _nextchar

	LWI(numbuf(SB), rSI)
	CALL16(BIOSputs(SB))

	POPR(rDI)
	POPR(rCX)
	POPR(rBX)
	POPR(rAX)

	CALL16(printspace(SB))
	RET

TEXT printDXAX(SB), $0
	PUSHR(rAX)
	MW(rDX, rAX)
	CALL16(printAX(SB))
	POPR(rAX)
	CALL16(printAX(SB))
	RET

TEXT printBX(SB), $0
	PUSHR(rAX)
	MW(rBX, rAX)
	CALL16(printAX(SB))
	POPR(rAX)
	RET

TEXT error(SB), $0
	BYTE $'E';

TEXT ioerror(SB), $0
	BYTE $'I';

TEXT nl(SB), $0
	BYTE $'\r';
	BYTE $'\n';
	BYTE $'\z';

TEXT numbuf(SB), $0
	BYTE $'X'; BYTE $'X'; BYTE $'X'; BYTE $'X';
	BYTE $'\z';

TEXT space(SB), $0
	BYTE $' ';
	BYTE $'\z';

TEXT sharp(SB), $0
	BYTE $'#'; BYTE $'\z';

/* "PBSR..." */
TEXT confidence(SB), $0
	BYTE $'P'; BYTE $'B'; BYTE $'S'; BYTE $'R';
	BYTE $'.'; BYTE $'.'; BYTE $'.';
	BYTE $'\z';
