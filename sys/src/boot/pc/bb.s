/*
 * Boot block. Loaded at 0x7C00
 *	8a bb.s; 8l -o bb -l -H3 -T0x7C00 bb.8
 */
/*
 * Can't write 16-bit code for 8a without getting into
 * lots of bother, so define some simple commands and
 * output the code directly.
 * This was a mistake in so many ways it's not true.
 * But I don't ever, ever have to do it again.
 */
#define rAX		0		/* rX  */
#define rCX		1
#define rDX		2
#define rBX		3
#define rSP		4		/* SP */
#define rBP		5		/* BP */
#define rSI		6		/* SI */
#define rDI		7		/* DI */

#define rAL		0		/* rL  */
#define rCL		1
#define rDL		2
#define rBL		3
#define rAH		4		/* rH */
#define rCH		5
#define rDH		6
#define rBH		7

#define rES		0		/* rS */
#define rCS		1
#define rSS		2
#define rDS		3
#define rFS		4
#define rGS		5

#define OP(o, m, r, rm)	BYTE $o;		/* op + modr/m byte */	\
			BYTE $((m<<6)|(r<<3)|rm)
#define OPrm(o, r, m)	OP(o, 0x00, r, 0x06);	/* general r <-> m */	\
			WORD $m;
#define OPrr(o, r0, r1)	OP(o, 0x03, r0, r1);	/* general r -> r */	\

#define LW(m, rX)	OPrm(0x8B, rX, m)	/* m -> rX */
#define LBPW(x, r)	OP(0x8B, 0x02, r, 0x06);/* x(rBP) -> r */	\
			WORD $x
#define LB(m, rB)	OPrm(0x8A, rB, m)	/* m -> r[HL] */
#define LBPB(x, r)	OP(0x8A, 0x01, r, 0x06);/* x(rBP) -> r */	\
			BYTE $x
#define SW(rX, m)	OPrm(0x89, rX, m)	/* rX -> m */
#define SBPW(r, x)	OP(0x89, 0x02, r, 0x06);/* r -> x(rBP) */	\
			WORD $x
#define STB(rB, m)	OPrm(0x88, rB, m)	/* rB -> m */
#define SBPB(r, x)	OP(0x88, 0x01, r, 0x06);/* r -> x(rBP) */	\
			BYTE $x
#define LWI(i, rX)	BYTE $(0xB8+rX);	/* i -> rX */		\
			WORD $i;
#define LBI(i, rB)	BYTE $(0xB0+rB);	/* i -> r[HL] */	\
			BYTE $i

#define MW(r0, r1)	OPrr(0x89, r0, r1)	/* r0 -> r1 */
#define MFSR(rS, rX)	OPrr(0x8C, rS, rX)	/* rS -> rX */
#define MTSR(rX, rS)	OPrr(0x8E, rS, rX)	/* rX -> rS */

#define ADC(r0, r1)	OPrr(0x11, r0, r1)	/* r0 + r1 -> r1 */
#define ADD(r0, r1)	OPrr(0x01, r0, r1)	/* r0 + r1 -> r1 */
#define AND(r0, r1)	OPrr(0x21, r0, r1)	/* r0 & r1 -> r1 */
#define ANDI(i, r)	OP(0x81, 0x03, 0x04, r);/* i & r -> r */	\
			WORD $i;
#define CLR(r)		OPrr(0x31, r, r)	/* r^r -> r */
#define CLRB(r)		OPrr(0x30, r, r)	/* r^r -> r */
#define CMP(r0, r1)	OPrr(0x39, r0, r1)	/* r1 - r0 -> flags */
#define CMPB(r0, r1)	OPrr(0x38, r0, r1)	/* r1 - r0 -> flags */
#define DEC(r)		BYTE $(0x48|r)		/* r-1 -> r */
#define DIV(r)		OPrr(0xF7, 0x06, r)	/* rAX/r -> rDX:rAX */
#define INC(r)		BYTE $(0x40|r)		/* r+1 -> r */
#define MUL(r)		OPrr(0xF7, 0x04, r)	/* r * rAX -> rDX:rAX */
#define OR(r0, r1)	OPrr(0x09, r0, r1)	/* r0|r1 -> r1 */
#define ORB(r0, r1)	OPrr(0x08, r0, r1)	/* r0|r1 -> r1 */
#define ROLI(i, r)	OPrr(0xC1, 0x00, r);	/* r<<>>i -> r */	\
			BYTE $i;
#define SHLI(i, r)	OPrr(0xC1, 0x04, r);	/* r<<i -> r */		\
			BYTE $i;
#define SHLBI(i, r)	OPrr(0xC0, 0x04, r);	/* r<<i -> r */		\
			BYTE $i;

#define CALL(f)		LWI(f, rDI);		/* &f -> rDI */		\
			BYTE $0xFF;		/* (*rDI) */		\
			BYTE $0xD7;
#define FARJUMP(s, o)	BYTE $0xEA;		/* far jump to s:o */	\
			WORD $o; WORD $s
#define	DELAY		BYTE $0xEB;		/* jmp .+2 */		\
			BYTE $0x00
#define SYSCALL(b)	INT $b			/* INT $b */

#define POKEW		BYTE $0x26;		/* MOVW	rAX, rES:[rBX] */	\
			BYTE $0x89; BYTE $0x07
#define OUTB(p, d)	LBI(d, rAL);		/* d -> I/O port p */	\
			BYTE $0xE6;					\
			BYTE $p; DELAY
#define PUSHA		BYTE $0x60
#define PUSHW(r)	BYTE $(0x50|r)		/* r  -> (--rSP) */
#define POPA		BYTE $0x61
#define POPW(r)		BYTE $(0x58|r)		/* (rSP++) -> r */
#define NOP		BYTE $0x90		/* nop */

/*
 * Now on to the real work.
 * If you're stilll reading, you have a
 * stomach of steel.
 */
#define RDIRBUF		0x00200		/* where to read the root directory (offset) */
#define CODEBUF		0x10000		/* where to load code (64Kb) */

/*
 * MS-DOS directory entry.
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
	MFSR(rCS, rAX)			/* set the data and stack segments */
	MTSR(rAX, rDS)
	MTSR(rAX, rSS)
	MTSR(rAX, rES)
	LWI(_magic-Xtotal(SB), rSP)
	MW(rSP, rBP)			/* set the indexed-data pointer */

	SBPB(rDL, Xdrive)		/* save the boot drive */

	STI

	LWI(confidence(SB), rSI)	/* for that warm, fuzzy feeling */
	CALL(BIOSputs(SB))

	CLR(rAX)			/* rAH == 0 == reset disc system */
	PUSHW(rBP)			/* may be trashed by SYSCALL */
	SYSCALL(0x13)			/* rDL == boot drive */
	POPW(rBP)
	ORB(rAH, rAH)			/* status (0 == successful completion) */
	JEQ _jmp00			/* can't rely on 8l doing other than short jumps */
	CALL(buggery(SB))

_jmp00:
	CLR(rAX)

	LB(_nfats(SB), rAL)		/* calculate root directory offset */
	LW(_fatsize(SB), rCX)
	MUL(rCX)
	LW(_nhiddenlo(SB), rCX)
	ADD(rCX, rAX)
	LW(_nhiddenhi(SB), rCX)
	ADC(rCX, rDX)
	LW(_nresrv(SB), rCX)
	ADD(rCX, rAX)
	LWI(0x0000, rCX)		/* don't set flags */
	ADC(rCX, rDX)

	SBPW(rAX, Xrootlo)		/* save */
	SBPW(rDX, Xroothi)

	LWI(_magic+RDIRBUF(SB), rBX)
	CALL(BIOSread(SB))		/* read the root directory */

	LWI(_magic+RDIRBUF(SB), rDI)	/* compare first directory entry */
	LWI(bootfile(SB), rSI)
	LWI(Dattr, rCX)
	REP
	CMPSB
	JEQ _jmp02
	CALL(buggery(SB))

_jmp02:
	CLR(rBX)			/* a handy value */
	LW(_rootsize(SB), rAX)		/* calculate and save Xrootsz */
	LWI(Dirsz, rCX)
	MUL(rCX)
	LW(_sectsize(SB), rCX)
	DEC(rCX)
	ADD(rCX, rAX)
	ADC(rBX, rDX);
	LW(_sectsize(SB), rCX)
	DIV(rCX)
	SBPW(rAX, Xrootsz)

	LW(_magic+(RDIRBUF+Dstart)(SB), rAX)	/* calculate starting sector address */
	DEC(rAX)				/* that's just the way it is... */
	DEC(rAX)
	LB(_clustsize(SB), rCL)
	CLRB(rCH)
	MUL(rCX)
	LBPW(Xrootlo, rCX)
	ADD(rCX, rAX)
	LBPW(Xroothi, rCX)
	ADC(rCX, rDX)
	LBPW(Xrootsz, rCX)
	ADD(rCX, rAX)
	ADC(rBX, rDX);

	PUSHW(rAX)
	PUSHW(rDX)
	LW(_magic+(RDIRBUF+Dlengthlo)(SB), rAX)	/* calculate how many sectors to read */
	LW(_magic+(RDIRBUF+Dlengthhi)(SB), rDX)
	LW(_sectsize(SB), rCX)
	DEC(rCX)
	ADD(rCX, rAX)
	ADC(rBX, rDX);
	LW(_sectsize(SB), rCX)
	DIV(rCX)
	MW(rAX, rCX)
	POPW(rDX)
	POPW(rAX)

	LWI((CODEBUF/16), rBX)		/* address to load into (seg + offset) */
	MTSR(rBX, rES)
	LWI(0x0100, rBX)

_readboot:
	CALL(BIOSread(SB))		/* read the sector */

	LW(_sectsize(SB), rDI)		/* bump addresses/counts */
	ADD(rDI, rBX)
	INC(rAX)
	LWI(0x0000, rDI)		/* don't set flags */
	ADC(rDI, rDX)
	LOOP _readboot

	LWI((CODEBUF/16), rDI)		/* set rDS for loaded code */
	MTSR(rDI, rDS)
	FARJUMP((CODEBUF/16), 0x0100)	/* no deposit, no return */

TEXT buggery(SB), $0
	LWI(error(SB), rSI)
	CALL(BIOSputs(SB))

_wait:
	CLR(rAX)			/* wait for any key */
	SYSCALL(0x16)

_reset:
	CLR(rBX)			/* set ES segment for BIOS area */
	MTSR(rBX, rES)

	LWI(0x0472, rBX)		/* warm-start code address */
	LWI(0x1234, rAX)		/* warm-start code */
	POKEW				/* MOVW	AX, ES:[BX] */

	FARJUMP(0xFFFF, 0x0000)		/* reset */


/*
 * Read a sector from a disc. On entry:
 *   rDX:rAX	sector number
 *   rES:rBX	buffer address
 * For SYSCALL(0x13):
 *   rAH	0x02
 *   rAL	number of sectors to read (1)
 *   rCH	low 8 bits of cylinder
 *   rCL	high 2 bits of cylinder (7-6), sector (5-0)
 *   rDH	head
 *   rDL	drive
 *   rES:rBX	buffer address
 */
TEXT BIOSread(SB), $0
	PUSHA				/* may be trashed by SYSCALL */

	LW(_trksize(SB), rDI)		/* rDI is scratch */
	DIV(rDI)			/* track -> rAX, sector -> rDX */

	INC(rDX)			/* sector numbers are 1-based */
	MW(rDX, rCX)
	ANDI(0x003F, rCX)		/* form sector bits in rCL */

	CLR(rDX)
	LW(_nheads(SB), rDI)
	DIV(rDI)			/* cylinder -> rAX, head -> rDX */

	SHLBI(0x06, rAH)		/* form cylinder hi and lo bits */
	ROLI(0x08, rAX)			/* swap rA[HL] */
	OR(rAX, rCX)			/* rCH, rCL now set */

	SHLI(0x08, rDX)			/* form head */
	LBPB(Xdrive, rDL)		/* form drive */

	LWI(0x0201, rAX)		/* form command and sectors */
	SYSCALL(0x13)			/* CF set on failure */
	JCC _BIOSreadret
	CALL(buggery(SB))

_BIOSreadret:
	POPA
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
	SYSCALL(0x10)
	JMP _BIOSputs

_BIOSputsret:
	POPA
	RET

TEXT error(SB), $0
	BYTE $'N'; BYTE $'o'; BYTE $'t'; BYTE $' '; BYTE $'a'; BYTE $' '; BYTE $'b'; BYTE $'o';
	BYTE $'o'; BYTE $'t'; BYTE $'a'; BYTE $'b'; BYTE $'l'; BYTE $'e'; BYTE $' '; BYTE $'d';
	BYTE $'i'; BYTE $'s'; BYTE $'c'; BYTE $' '; BYTE $'o'; BYTE $'r'; BYTE $' '; BYTE $'d';
	BYTE $'i'; BYTE $'s'; BYTE $'c'; BYTE $' '; BYTE $'e'; BYTE $'r'; BYTE $'r'; BYTE $'o';
	BYTE $'r'; BYTE $'\r';BYTE $'\n';BYTE $'P'; BYTE $'r'; BYTE $'e'; BYTE $'s'; BYTE $'s';
	BYTE $' '; BYTE $'a'; BYTE $'l'; BYTE $'m'; BYTE $'o'; BYTE $'s'; BYTE $'t'; BYTE $' ';
	BYTE $'a'; BYTE $'n'; BYTE $'y'; BYTE $' '; BYTE $'k'; BYTE $'e'; BYTE $'y'; BYTE $' ';
	BYTE $'t'; BYTE $'o'; BYTE $' '; BYTE $'r'; BYTE $'e'; BYTE $'b'; BYTE $'o'; BYTE $'o'; 
	BYTE $'t'; BYTE $'.'; BYTE $'.'; BYTE $'.';
	BYTE $'\z';

TEXT bootfile(SB), $0
	BYTE $'B'; BYTE $' '; BYTE $' '; BYTE $' '; BYTE $' '; BYTE $' '; BYTE $' '; BYTE $' ';
	BYTE $'C'; BYTE $'O'; BYTE $'M';
	BYTE $'\z';

TEXT confidence(SB), $0
	BYTE $'B'; BYTE $'o'; BYTE $'o'; BYTE $'t'; BYTE $'i'; BYTE $'n'; BYTE $'g'; BYTE $'.';
	BYTE $'.'; BYTE $'.'; BYTE $'\r';BYTE $'\n';
	BYTE $'\z';
