/*
 * Debugging boot sector.  Reads the first directory
 * sector from disk and displays it.
 *
 * It relies on the _volid field in the FAT header containing
 * the LBA of the root directory.
 */
#include "x16.h"

#define DIROFF		0x00200		/* where to read the root directory (offset) */
#define LOADSEG		(0x10000/16)	/* where to load code (64KB) */
#define LOADOFF		0

/*
 * FAT directory entry.
 */
#define Dname		0x00
#define Dext		0x08
#define Dattr		0x0B
#define Dtime		0x16
#define Ddate		0x18
#define Dstart		0x1A
#define Dlengthlo	0x1C
#define Dlengthhi	0x1E

#define Dirsz		0x20

/*
 * We keep data on the stack, indexed by rBP.
 */
#define Xdrive		0x00		/* boot drive, passed by BIOS in rDL */
#define Xrootlo		0x02		/* offset of root directory */
#define Xroothi		0x04
#define Xrootsz		0x06		/* file data area */
#define Xtotal		0x08		/* sum of allocated data above */
#define Xdap		0x00		/* disc address packet */

TEXT _magic(SB), $0
	BYTE $0xEB; BYTE $0x3C;		/* jmp .+ 0x3C  (_start0x3E) */
	BYTE $0x90			/* nop */
TEXT _version(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00;
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
TEXT _sectsize(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _clustsize(SB), $0
	BYTE $0x00
TEXT _nresrv(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _nfats(SB), $0
	BYTE $0x00
TEXT _rootsize(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _volsize(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _mediadesc(SB), $0
	BYTE $0x00
TEXT _fatsize(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _trksize(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _nheads(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _nhiddenlo(SB), $0
	BYTE $0x00; BYTE $0x00
TEXT _nhiddenhi(SB), $0
	BYTE $0x00; BYTE $0x00;
TEXT _bigvolsize(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00;
TEXT _driveno(SB), $0
	BYTE $0x00
TEXT _reserved0(SB), $0
	BYTE $0x00
TEXT _bootsig(SB), $0
	BYTE $0x00
TEXT _volid(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00;
TEXT _label(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00;
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00
	BYTE $0x00; BYTE $0x00; BYTE $0x00
TEXT _type(SB), $0
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00;
	BYTE $0x00; BYTE $0x00; BYTE $0x00; BYTE $0x00;

_start0x3E:
	CLI
	CLR(rAX)
	MTSR(rAX, rSS)			/* 0000 -> rSS */
	MTSR(rAX, rDS)			/* 0000 -> rDS, source segment */
	MTSR(rAX, rES)
	LWI(_magic-Xtotal(SB), rSP)
	MW(rSP, rBP)			/* set the indexed-data pointer */
	SBPB(rDL, Xdrive)		/* save the boot drive */

	/* VMware starts us at 7C0:0.  Move to 0:7C00 */
	PUSHR(rAX)
	LWI(_nxt(SB), rAX)
	PUSHR(rAX)
	BYTE $0xCB	/* FAR RET */

TEXT _nxt(SB), $0
	STI
	LWI(confidence(SB), rSI)	/* for that warm, fuzzy feeling */
	CALL16(BIOSputs(SB))

	CALL16(dreset(SB))

_jmp00:
	LW(_volid(SB), rAX)		/* Xrootlo */
	LW(_volid+2(SB), rDX)		/* Xroothi */

	LWI(_magic+DIROFF(SB), rBX)
	CALL16(BIOSread(SB))		/* read the root directory */

	CALL16(printnl(SB))
	LWI(_magic+DIROFF(SB), rBX)
	LWI((512/2), rCX)
	CALL16(printbuf(SB))

xloop:
	JMP xloop


TEXT buggery(SB), $0
	LWI(error(SB), rSI)
	CALL16(BIOSputs(SB))

TEXT quietbuggery(SB), $0
xbuggery:
	JMP xbuggery

/*
 * Read a sector from a disc. On entry:
 *   rDX:rAX	sector number
 *   rES:rBX	buffer address
 * For BIOSCALL(0x13):
 *   rAH	0x02
 *   rAL	number of sectors to read (1)
 *   rCH	low 8 bits of cylinder
 *   rCL	high 2 bits of cylinder (7-6), sector (5-0)
 *   rDH	head
 *   rDL	drive
 *   rES:rBX	buffer address
 */
TEXT BIOSread(SB), $0
	LWI(5, rDI)			/* retry count (ATAPI ZIPs suck) */
_retry:
	PUSHA				/* may be trashed by BIOSCALL */
	PUSHR(rBX)

	LW(_trksize(SB), rBX)
	LW(_nheads(SB), rDI)
	IMUL(rDI, rBX)
	OR(rBX, rBX)
	JZ _ioerror

_okay:
	DIV(rBX)			/* cylinder -> rAX, track,sector -> rDX */

	MW(rAX, rCX)			/* save cylinder */
	ROLI(0x08, rCX)			/* swap rC[HL] */
	SHLBI(0x06, rCL)		/* move high bits up */

	MW(rDX, rAX)
	CLR(rDX)
	LW(_trksize(SB), rBX)

	DIV(rBX)			/* head -> rAX, sector -> rDX */

	INC(rDX)			/* sector numbers are 1-based */
	ANDI(0x003F, rDX)		/* should not be necessary */
	OR(rDX, rCX)

	MW(rAX, rDX)
	SHLI(0x08, rDX)			/* form head */
	LBPB(Xdrive, rDL)		/* form drive */

	POPR(rBX)
	LWI(0x0201, rAX)		/* form command and sectors */
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
	JMP xbuggery

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
	PUSHW(rAX)
	PUSHW(rBX)
	PUSHW(rCX)
	PUSHW(rDI)

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

	POPW(rDI)
	POPW(rCX)
	POPW(rBX)
	POPW(rAX)

	CALL16(printspace(SB))
	RET

TEXT printDXAX(SB), $0
	PUSHW(rAX)
	MW(rDX, rAX)
	CALL16(printAX(SB))
	POPW(rAX)
	CALL16(printAX(SB))
	RET

TEXT printBX(SB), $0
	PUSHW(rAX)
	MW(rBX, rAX)
	CALL16(printAX(SB))
	POPW(rAX)
	RET

/*
 * Output some number of words to the display
 * rDS:rDI - buffer
 * rCX: number of words
 */
TEXT printbuf(SB), $0
	PUSHW(rAX)
	PUSHW(rBX)
	PUSHW(rCX)

_nextword:
	LXW(0, xBX, rAX)
	CALL16(printAX(SB))
	INC(rBX)
	INC(rBX)
	DEC(rCX)
	JNE _nextword

	POPW(rCX)
	POPW(rBX)
	POPW(rAX)
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

TEXT confidence(SB), $0
	BYTE $'P'; BYTE $'\z'
