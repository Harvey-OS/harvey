/*
 * Hard disc boot block. Loaded at 0x7C00, relocates to 0x0600:
 *	8a mbr.s; 8l -o mbr -l -H3 -T0x0600 mbr.8
 */
#include "x16.h"
#include "mem.h"

/*#define FLOPPY	1		/* test on a floppy */
#define TRACE(C)	PUSHA;\
			CLR(rBX);\
			MOVB $C, AL;\
			LBI(0x0E, rAH);\
			BIOSCALL(0x10);\
			POPA

/*
 * We keep data on the stack, indexed by BP.
 */
#define Xdap		0x00		/* disc address packet */
#define Xtable		0x10		/* partition table entry */
#define Xdrive		0x12		/* starting disc */
#define Xtotal		0x14		/* sum of allocated data above */

/*
 * Start: loaded at 0000:7C00, relocate to 0000:0600.
 * Boot drive is in rDL.
 */
TEXT _start(SB), $0
	CLI
	CLR(rAX)
	MTSR(rAX, rSS)			/* 0000 -> rSS */
	LWI((0x7C00-Xtotal), rSP)	/* 7Bxx -> rSP */
	MW(rSP, rBP)			/* set the indexed-data pointer */

	MTSR(rAX, rDS)			/* 0000 -> rDS, source segment */
	LWI(0x7C00, rSI)		/* 7C00 -> rSI, source offset */
	MTSR(rAX, rES)			/* 0000 -> rES, destination segment */
	LWI(0x600, rDI)			/* 0600 -> rDI, destination offset */
	LWI(0x100, rCX)			/* 0100 -> rCX, loop count (words) */

	CLD
	REP; MOVSL			/* MOV DS:[(E)SI] -> ES:[(E)DI] */

	FARJUMP16(0x0000, _start0600(SB))

TEXT _start0600(SB), $0
#ifdef FLOPPY
	LBI(0x80, rDL)
#else
	CLRB(rAL)			/* some systems pass 0 */
	CMPBR(rAL, rDL)
	JNE _save
	LBI(0x80, rDL)
#endif /* FLOPPY */
_save:
	SXB(rDL, Xdrive, xBP)		/* save disc */

	LWI(confidence(SB), rSI)	/* for that warm, fuzzy feeling */
	CALL16(BIOSputs(SB))

	LWI(_start+0x01BE(SB), rSI)	/* address of partition table */
	LWI(0x04, rCX)			/* 4 entries in table */
	LBI(0x80, rAH)			/* active entry value */
	CLRB(rAL)			/* inactive entry value */

_activeloop0:
	LXB(0x00, xSI, rBL)		/* get active entry from table */
	CMPBR(rBL, rAH)			/* is this an active entry? */
	JEQ _active

	CMPBR(rBL, rAL)			/* if not active it should be 0 */
	JNE _invalidMBR

	ADDI(0x10, rSI)			/* next table entry */
	DEC(rCX)
	JNE _activeloop0

	LWI(noentry(SB), rSI)
	CALL16(buggery(SB))

_active:
	MW(rSI, rDI)			/* save table address */

_activeloop1:
	ADDI(0x10, rSI)			/* next table entry */
	DEC(rCX)
	JEQ _readsector

	LXB(0x00, xSI, rBL)		/* get active entry from table */
	CMPBR(rBL, rAH)			/* is this an active entry? */
	JNE _activeloop1		/* should only be one active */

_invalidMBR:
	LWI(invalidMBR(SB), rSI)
	CALL16(buggery(SB))

_readsector:
	LBI(0x41, rAH)			/* check extensions present */
	LWI(0x55AA, rBX)
	LXB(Xdrive, xBP, rDL)		/* drive */
	BIOSCALL(0x13)			/* CF set on failure */
	JCS _readsector2
	CMPI(0xAA55, rBX)
	JNE _readsector2
	ANDI(0x0001, rCX)
	JEQ _readsector2

_readsector42:
	SBPBI(0x10, Xdap+0)		/* packet size */
	SBPBI(0x00, Xdap+1)		/* reserved */
	SBPBI(0x01, Xdap+2)		/* number of blocks to transfer */
	SBPBI(0x00, Xdap+3)		/* reserved */
	SBPWI(0x7C00, Xdap+4)		/* transfer buffer :offset */
	SBPWI(0x0000, Xdap+6)		/* transfer buffer seg: */
	LXW(0x08, xDI, rAX)		/* LBA (64-bits) */
	SBPW(rAX, Xdap+8)
	LXW(0x0A, xDI, rAX)
	SBPW(rAX, Xdap+10)
	SBPWI(0x0000, Xdap+12)
	SBPWI(0x0000, Xdap+14)

	MW(rBP, rSI)			/* disk address packet */
	LBI(0x42, rAH)			/* extended read */
	BIOSCALL(0x13)			/* CF set on failure */
	JCC _readsectorok

	LWI(ioerror(SB), rSI)
	CALL16(buggery(SB))

/*
 * Read a sector from a disc using the traditional BIOS call.
 * For BIOSCALL(0x13/AH=0x02):
 *   rAH	0x02
 *   rAL	number of sectors to read (1)
 *   rCH	low 8 bits of cylinder
 *   rCL	high 2 bits of cylinder (7-6), sector (5-0)
 *   rDH	head
 *   rDL	drive
 *   rES:rBX	buffer address
 */
_readsector2:
	LXB(0x01, xDI, rDH)		/* head */
	LXW(0x02, xDI, rCX)		/* save active cylinder/sector */

	LWI(0x0201, rAX)		/* read one sector */
	LXB(Xdrive, xBP, rDL)		/* drive */
	LWI(0x7C00, rBX)		/* buffer address (rES already OK) */
	BIOSCALL(0x13)			/* CF set on failure */
	JCC _readsectorok

	LWI(ioerror(SB), rSI)
	CALL16(buggery(SB))

_readsectorok:
	LWI(0x7C00, rBX)		/* buffer address (rES already OK) */
	LXW(0x1FE, xBX, rAX)
	CMPI(0xAA55, rAX)
	JNE _bbnotok

	/*
	 * Jump to the loaded PBS.
	 * rDL and rSI should still contain the drive
	 * and partition table pointer respectively.
	 */
	MW(rDI, rSI)
	FARJUMP16(0x0000, 0x7C00)

_bbnotok:
	LWI(invalidPBS(SB), rSI)

TEXT buggery(SB), $0
	CALL16(BIOSputs(SB))
	LWI(reboot(SB), rSI)
	CALL16(BIOSputs(SB))

_wait:
	CLR(rAX)			/* wait for any key */
	BIOSCALL(0x16)

_reset:
	CLR(rBX)			/* set ES segment for BIOS area */
	MTSR(rBX, rES)

	LWI(0x0472, rBX)		/* warm-start code address */
	LWI(0x1234, rAX)		/* warm-start code */
	POKEW				/* MOVW	AX, ES:[BX] */

	FARJUMP16(0xFFFF, 0x0000)	/* reset */

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

/* "No active entry in MBR" */
TEXT noentry(SB), $0
	BYTE $'N'; BYTE $'o'; BYTE $' '; BYTE $'a';
	BYTE $'c'; BYTE $'t'; BYTE $'i'; BYTE $'v';
	BYTE $'e'; BYTE $' '; BYTE $'e'; BYTE $'n';
	BYTE $'t'; BYTE $'r'; BYTE $'y'; BYTE $' ';
	BYTE $'i'; BYTE $'n'; BYTE $' '; BYTE $'M';
	BYTE $'B'; BYTE $'R';
	BYTE $'\z';

/* "Invalid MBR" */
TEXT invalidMBR(SB), $0
	BYTE $'I'; BYTE $'n'; BYTE $'v'; BYTE $'a';
	BYTE $'l'; BYTE $'i'; BYTE $'d'; BYTE $' ';
	BYTE $'M'; BYTE $'B'; BYTE $'R';
	BYTE $'\z';

/* "I/O error" */
TEXT ioerror(SB), $0
	BYTE $'I'; BYTE $'/'; BYTE $'O'; BYTE $' ';
	BYTE $'e'; BYTE $'r'; BYTE $'r'; BYTE $'o';
	BYTE $'r';
	BYTE $'\z';

/* "Invalid PBS" */
TEXT invalidPBS(SB), $0
	BYTE $'I'; BYTE $'n'; BYTE $'v'; BYTE $'a';
	BYTE $'l'; BYTE $'i'; BYTE $'d'; BYTE $' ';
	BYTE $'P'; BYTE $'B'; BYTE $'S';
	BYTE $'\z';

/* "\r\nPress almost any key to reboot..." */
TEXT reboot(SB), $0
	BYTE $'\r';BYTE $'\n';
	BYTE $'P'; BYTE $'r'; BYTE $'e'; BYTE $'s';
	BYTE $'s'; BYTE $' '; BYTE $'a'; BYTE $'l'; 
	BYTE $'m'; BYTE $'o'; BYTE $'s'; BYTE $'t';
	BYTE $' '; BYTE $'a'; BYTE $'n'; BYTE $'y';
	BYTE $' '; BYTE $'k'; BYTE $'e'; BYTE $'y';
	BYTE $' '; BYTE $'t'; BYTE $'o'; BYTE $' ';
	BYTE $'r'; BYTE $'e'; BYTE $'b'; BYTE $'o';
	BYTE $'o'; BYTE $'t'; BYTE $'.'; BYTE $'.';
	BYTE $'.';
	BYTE $'\z';

/* "MBR..." */
TEXT confidence(SB), $0
	BYTE $'M'; BYTE $'B'; BYTE $'R'; BYTE $'.';
	BYTE $'.'; BYTE $'.';
	BYTE $'\z';
