/*
 * FAT Partition Boot Sector. Loaded at 0x7C00:
 *	8a pbslba.s; 8l -o pbslba -l -H3 -T0x7C00 pbslba.8
 * Will load the target at LOADSEG*16+LOADOFF, so the target
 * should be probably be loaded with LOADOFF added to the
 * -Taddress.
 * If LOADSEG is a multiple of 64KB and LOADOFF is 0 then
 * targets larger than 64KB can be loaded.
 *
 * This code is uses Enhanced BIOS Services for Disc Drives and
 * can be used with discs up to 137GB in capacity.
 *
 * It relies on the _volid field in the FAT header containing
 * the LBA of the root directory.
 */
#include "x16.h"
#include "mem.h"

#define LOADSEG		(0x10000/16)	/* where to load code (64KB) */
#define LOADOFF		0
#define DIROFF		0x0200		/* where to read the root directory */

/*
 * FAT directory entry.
 */
#define Dname		0x00
#define Dnamesz	0x0B
#define Dext		0x08
#define Dattr		0x0B
#define Dtime		0x16
#define Ddate		0x18
#define Dstart		0x1A
#define Dlengthlo	0x1C
#define Dlengthhi	0x1E

#define Dirsz		0x20

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

	/* booting from a CD starts us at 7C0:0.  Move to 0:7C00 */
	PUSHR(rAX)
	LWI(_nxt(SB), rAX)
	PUSHR(rAX)
	BYTE $0xCB	/* FAR RET */

TEXT _nxt(SB), $0
	STI

	LWI(confidence(SB), rSI)	/* for that warm, fuzzy feeling */
	CALL16(BIOSputs(SB))

	LBI(0x41, rAH)			/* check extensions present */
	LWI(0x55AA, rBX)
	LXB(Xdrive, xBP, rDL)		/* drive */
	BIOSCALL(0x13)			/* CF set on failure */
	JCS _jmp01
	CMPI(0xAA55, rBX)
	JNE _jmp01
	ANDI(0x0001, rCX)
	JEQ _jmp01

					/* rCX contains 0x0001 */
	SBPWI(0x0010, Xdap+0)		/* reserved + packet size */
	SBPW(rCX, Xdap+2)		/* reserved + # of blocks to transfer */

	DEC(rCX)
	SBPW(rCX, Xdap+12)
	SBPW(rCX, Xdap+14)

	CALL16(dreset(SB))

_jmp00:
	LW(_volid(SB), rAX)		/* Xrootlo */
	LW(_volid+2(SB), rDX)		/* Xroothi */

	LWI(_magic+DIROFF(SB), rBX)
	CALL16(BIOSread(SB))		/* read the root directory */

	LWI((512/Dirsz), rBX)

	LWI(_magic+DIROFF(SB), rDI)	/* compare first directory entry */

_cmp00:
	PUSHR(rDI)			/* save for later if it matches */
	LWI(bootfile(SB), rSI)
	LWI(Dnamesz, rCX)
	REP
	CMPSB
	POPR(rDI)
	JEQ _jmp02

	DEC(rBX)
	JEQ _jmp01

	ADDI(Dirsz, rDI)
	JMP _cmp00
_jmp01:
	CALL16(buggery(SB))

_jmp02:
	CLR(rBX)			/* a handy value */
	LW(_rootsize(SB), rAX)		/* calculate and save Xrootsz */
	LWI(Dirsz, rCX)
	MUL(rCX)
	LW(_sectsize(SB), rCX)
	PUSHR(rCX)
	DEC(rCX)
	ADD(rCX, rAX)
	ADC(rBX, rDX)
	POPR(rCX)			/* _sectsize(SB) */
	DIV(rCX)
	PUSHR(rAX)			/* Xrootsz */

	/*
	 * rDI points to the matching directory entry.
	 */
	LXW(Dstart, xDI, rAX)		/* starting sector address */
	DEC(rAX)			/* that's just the way it is */
	DEC(rAX)
	LB(_clustsize(SB), rCL)
	CLRB(rCH)
	MUL(rCX)
	LW(_volid(SB), rCX)		/* Xrootlo */
	ADD(rCX, rAX)
	LW(_volid+2(SB), rCX)		/* Xroothi */
	ADC(rCX, rDX)
	POPR(rCX)			/* Xrootsz */
	ADD(rCX, rAX)
	ADC(rBX, rDX)

	PUSHR(rAX)			/* calculate how many sectors to read */
	PUSHR(rDX)
	LXW(Dlengthlo, xDI, rAX)
	LXW(Dlengthhi, xDI, rDX)
	LW(_sectsize(SB), rCX)
	PUSHR(rCX)
	DEC(rCX)
	ADD(rCX, rAX)
	ADC(rBX, rDX)
	POPR(rCX)			/* _sectsize(SB) */
	DIV(rCX)
	MW(rAX, rCX)
	POPR(rDX)
	POPR(rAX)

	LWI(LOADSEG, rBX)		/* address to load into (seg+offset) */
	MTSR(rBX, rES)			/*  seg */
	LWI(LOADOFF, rBX)		/*  offset */

_readboot:
	CALL16(BIOSread(SB))		/* read the sector */

	LW(_sectsize(SB), rDI)		/* bump addresses/counts */
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

/* "Bad format or I/O error\r\nPress almost any key to reboot..." */
TEXT error(SB), $0
	BYTE $'B'; BYTE $'a'; BYTE $'d'; BYTE $' ';
	BYTE $'f'; BYTE $'o'; BYTE $'r'; BYTE $'m';
	BYTE $'a'; BYTE $'t'; BYTE $' '; BYTE $'o';
	BYTE $'r'; BYTE $' ';
/* "I/O error\r\nPress almost any key to reboot..." */
TEXT ioerror(SB), $0
	BYTE $'I'; BYTE $'/'; BYTE $'O'; BYTE $' ';
	BYTE $'e'; BYTE $'r'; BYTE $'r'; BYTE $'o';
	BYTE $'r'; BYTE $'\r';BYTE $'\n';
	BYTE $'P'; BYTE $'r'; BYTE $'e'; BYTE $'s';
	BYTE $'s'; BYTE $' '; BYTE $'a'; BYTE $' ';
	BYTE $'k'; BYTE $'e'; BYTE $'y';
	BYTE $' '; BYTE $'t'; BYTE $'o'; BYTE $' ';
	BYTE $'r'; BYTE $'e'; BYTE $'b'; BYTE $'o';
	BYTE $'o'; BYTE $'t';
	BYTE $'.'; BYTE $'.'; BYTE $'.';
	BYTE $'\z';

#ifdef USEBCOM
/* "B       COM" */
TEXT bootfile(SB), $0
	BYTE $'B'; BYTE $' '; BYTE $' '; BYTE $' ';
	BYTE $' '; BYTE $' '; BYTE $' '; BYTE $' ';
	BYTE $'C'; BYTE $'O'; BYTE $'M';
	BYTE $'\z';
#else
/* "9LOAD      " */
TEXT bootfile(SB), $0
	BYTE $'9'; BYTE $'L'; BYTE $'O'; BYTE $'A';
	BYTE $'D'; BYTE $' '; BYTE $' '; BYTE $' ';
	BYTE $' '; BYTE $' '; BYTE $' ';
	BYTE $'\z';
#endif /* USEBCOM */

/* "PBS..." */
TEXT confidence(SB), $0
	BYTE $'P'; BYTE $'B'; BYTE $'S'; BYTE $'2';
	BYTE $'.'; BYTE $'.'; BYTE $'.';
	BYTE $'\z';
