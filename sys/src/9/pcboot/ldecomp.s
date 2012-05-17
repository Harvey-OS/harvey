/*
 * Bootstrap loader decompressor.  Starts at 0x10000 (where pbs puts it)
 * or 0x7c00 (where pxe puts it) and memmoves kernel (immediately following)
 * into standard kernel location.
 */
#include "mem.h"
#include "/sys/src/boot/pc/x16.h"

#undef BIOSCALL		/* we don't know what evil the bios gets up to */
#define BIOSCALL(b)	INT $(b); CLI

#define CMPL(r0, r1)	BYTE $0x66; CMP(r0, r1)
#define LLI(i, rX)	BYTE $0x66;		/* i -> rX */		\
			BYTE $(0xB8+rX);				\
			LONG $i;
#define CPUID		BYTE $0x0F; BYTE $0xA2	/* CPUID, argument in AX */
#define WBINVD		BYTE $0x0F; BYTE $0x09

TEXT origin(SB), $0
/*
 *	turn off interrupts
 */
	CLI

	/*
	 * This part of l.s is used only in the boot kernel.
	 * It assumes that we are in real address mode, i.e.,
	 * that we look like an 8086.
	 *
	 * Make sure the segments are reasonable.
	 * If we were started directly from the BIOS
	 * (i.e. no MS-DOS) then DS may not be
	 * right.
	 */
	MOVW	CS, AX
	MOVW	AX, DS

	LWI(0, rAX)			/* always put stack in first 64k */
	MTSR(rAX, rSS)
	LWI(origin(SB), rSP)		/* set the stack pointer */

	LWI(0x2401, rAX)		/* enable a20 line */
	BIOSCALL(0x15)

	XORL	AX, AX
	MOVB	$0x03, AL
//	LWI(3, rAX)
	INT	$0x10			/* set video mode in AL */

/*
 * Check for CGA mode.
 */
_cgastart:
	LWI(0x0F00, rAX)		/* get current video mode in AL */
	BIOSCALL(0x10)
	ANDI(0x007F, rAX)
	SUBI(0x0003, rAX)		/* is it mode 3? */
	JEQ	_cgamode3

	LWI(0x0003, rAX)		/* turn on text mode 3 */
	BIOSCALL(0x10)
_cgamode3:
	LWI(_hello(SB), rSI)
	CALL	_cgaputs(SB)

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
 * 	goto protected mode
 */
/*	MOVL	loadgdtptr(SB),GDTR /**/
	 BYTE	$0x0f
	 BYTE	$0x01
	 BYTE	$0x16
	 WORD	$loadgdtptr(SB)

	DELAY
	LWI(1, rAX)
	/* MOV AX,MSW */
	BYTE $0x0F; BYTE $0x01; BYTE $0xF0

/*
 *	clear prefetch queue (weird code to avoid optimizations)
 */
	DELAY

/*
 *	set all segs
 */
/*	MOVW	$KDSEL,AX	/**/
	 BYTE	$0xc7
	 BYTE	$0xc0
	 WORD	$KDSEL
	MOVW	AX,DS
	MOVW	AX,SS
	MOVW	AX,ES
	MOVW	AX,FS
	MOVW	AX,GS

	MOVW	$(20*1024*1024-4), SP		/* new stack pointer */
	DELAY

	/* god only knows what the damned bios has been up to... */
	CLI

	/* jump to C (main) */
/*	JMPFAR	KESEL:$main(SB) /**/
	 BYTE	$0x66
	 BYTE	$0xEA
	 LONG	$_main(SB)
	 WORD	$KESEL

/* output a cheery wee message (rSI) */
TEXT _cgaputs(SB), $0
//_cgaputs:
	CLR(rBX)
_cgaputsloop:
	LODSB
	ORB(rAL, rAL)
	JEQ	_cgaend

	LBI(0x0E,rAH)
	BIOSCALL(0x10)
	JMP	_cgaputsloop
_cgaend:
	RET

TEXT _hello(SB), $0
	BYTE $'\r'; BYTE $'\n'
	BYTE $'9'; BYTE $'b'; BYTE $'o'; BYTE $'o'
	BYTE $'t'; BYTE $' '
	BYTE $'\z'

/* offset into bios tables using segment ES.  stos? use (es,di) */
TEXT _DI(SB), $0
	LONG	$0

/* segment address of bios tables (BIOSTABLES >> 4) */
TEXT _ES(SB), $0
	LONG	$0

/*
 *  pointer to initial gdt
 */
TEXT	loadgdtptr(SB),$0
	WORD	$(4*8)
	LONG	$loadgdt(SB)

/*
 *  gdt to get us to 32-bit/segmented/unpaged mode
 */
TEXT	loadgdt(SB),$0

	/* null descriptor */
	LONG	$0
	LONG	$0

	/* data segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(0xFFFF)
	LONG	$(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)

	/* exec segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(0xFFFF)
	LONG	$(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

	/* exec segment descriptor for 4 gigabytes (PL 0) 16-bit */
	LONG	$(0xFFFF)
	LONG	$(SEGG|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

/*
 *  output a byte
 */
TEXT	outb(SB),$0
	MOVL	p+0(FP),DX
	MOVL	b+4(FP),AX
	OUTB
	RET

/*
 *  input a byte
 */
TEXT	inb(SB),$0
	MOVL	p+0(FP),DX
	XORL	AX,AX
	INB
	RET

TEXT mb586(SB), $0
	XORL	AX, AX
	CPUID
	RET

TEXT wbinvd(SB), $0
	WBINVD
	RET

TEXT	splhi(SB),$0
	PUSHFL
	POPL	AX
	CLI
	RET
