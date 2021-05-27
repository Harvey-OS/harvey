/*
 * Common startup and coprocessor instructions for armv6 and armv7
 * The rest of l.s has been moved to armv[67].s
 */

#include "arm.s"

/*
 * on bcm2836, only cpu0 starts here
 * other cpus enter at cpureset in armv7.s
 */
TEXT _start(SB), 1, $-4
	/*
	 * load physical base for SB addressing while mmu is off
	 * keep a handy zero in R0 until first function call
	 */
	MOVW	$setR12(SB), R12
	SUB	$KZERO, R12
	ADD	$PHYSDRAM, R12
	MOVW	$0, R0

	/*
	 * start stack at top of mach (physical addr)
	 */
	MOVW	$PADDR(MACHADDR+MACHSIZE-4), R13

	/*
	 * do arch-dependent startup (no return)
	 */
	BL	,armstart(SB)
	B	,0(PC)

	RET

/*
 * coprocessor instructions for local timer (armv7)
 */
TEXT	cprdfeat1(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpID), C(CpIDfeat), 1
	RET
TEXT	cpwrtimerphysctl(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpTIMER), C(CpTIMERphys), CpTIMERphysctl
	RET
TEXT	cpwrtimerphysval(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpTIMER), C(CpTIMERphys), CpTIMERphysval
	RET

/*
 * coprocessor instructions for vfp3
 */

#define VSTR(fpreg,Rn,off) \
  WORD $(0xed800000 | (fpreg&15)<<12 | ((fpreg&16)<<(22-4)) | Rn<<16 | CpDFP<<8 | (off))
#define VLDR(fpreg,Rn,off) \
  WORD $(0xed900000 | (fpreg&15)<<12 | ((fpreg&16)<<(22-4)) | Rn<<16 | CpDFP<<8 | (off))
#define VMSR(fpctlreg,Rt)	WORD $(0xeee00010 | fpctlreg<<16 | Rt<<12 | CpFP<<8)
#define VMRS(Rt,fpctlreg)	WORD $(0xeef00010 | fpctlreg<<16 | Rt<<12 | CpFP<<8)
#define Fpsid	0
#define Fpscr	1
#define Fpexc	8

TEXT	cprdcpaccess(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpCPaccess
	RET
TEXT	cpwrcpaccess(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpCPaccess
	BARRIERS
	RET
TEXT	fprdsid(SB), 1, $-4
	VMRS(0,Fpsid)
	RET
TEXT	fprdscr(SB), 1, $-4
	VMRS(0,Fpscr)
	RET
TEXT	fprdexc(SB), 1, $-4
	VMRS(0,Fpexc)
	RET
TEXT	fpwrscr(SB), 1, $-4
	VMSR(Fpscr,0)
	RET
TEXT	fpwrexc(SB), 1, $-4
	VMSR(Fpexc,0)
	RET

TEXT	fpsaveregs(SB), 1, $-4
	MOVW	R0, R1		/* dest */
	MOVW	4(FP), R2	/* reg count */
	VSTR(0, 1, 0*2)
	VSTR(1, 1, 1*2)
	VSTR(2, 1, 2*2)
	VSTR(3, 1, 3*2)
	VSTR(4, 1, 4*2)
	VSTR(5, 1, 5*2)
	VSTR(6, 1, 6*2)
	VSTR(7, 1, 7*2)
	VSTR(8, 1, 8*2)
	VSTR(9, 1, 9*2)
	VSTR(10, 1, 10*2)
	VSTR(11, 1, 11*2)
	VSTR(12, 1, 12*2)
	VSTR(13, 1, 13*2)
	VSTR(14, 1, 14*2)
	VSTR(15, 1, 15*2)
	CMP	$16, R2
	BEQ	fpsavex
	VSTR(16, 1, 16*2)
	VSTR(17, 1, 17*2)
	VSTR(18, 1, 18*2)
	VSTR(19, 1, 19*2)
	VSTR(20, 1, 20*2)
	VSTR(21, 1, 21*2)
	VSTR(22, 1, 22*2)
	VSTR(23, 1, 23*2)
	VSTR(24, 1, 24*2)
	VSTR(25, 1, 25*2)
	VSTR(26, 1, 26*2)
	VSTR(27, 1, 27*2)
	VSTR(28, 1, 28*2)
	VSTR(29, 1, 29*2)
	VSTR(30, 1, 30*2)
	VSTR(31, 1, 31*2)
fpsavex:
	RET

TEXT	fprestregs(SB), 1, $-4
	MOVW	R0, R1		/* src */
	MOVW	4(FP), R2	/* reg count */
	VLDR(0, 1, 0*2)
	VLDR(1, 1, 1*2)
	VLDR(2, 1, 2*2)
	VLDR(3, 1, 3*2)
	VLDR(4, 1, 4*2)
	VLDR(5, 1, 5*2)
	VLDR(6, 1, 6*2)
	VLDR(7, 1, 7*2)
	VLDR(8, 1, 8*2)
	VLDR(9, 1, 9*2)
	VLDR(10, 1, 10*2)
	VLDR(11, 1, 11*2)
	VLDR(12, 1, 12*2)
	VLDR(13, 1, 13*2)
	VLDR(14, 1, 14*2)
	VLDR(15, 1, 15*2)
	CMP	$16, R2
	BEQ	fprestx
	VLDR(16, 1, 16*2)
	VLDR(17, 1, 17*2)
	VLDR(18, 1, 18*2)
	VLDR(19, 1, 19*2)
	VLDR(20, 1, 20*2)
	VLDR(21, 1, 21*2)
	VLDR(22, 1, 22*2)
	VLDR(23, 1, 23*2)
	VLDR(24, 1, 24*2)
	VLDR(25, 1, 25*2)
	VLDR(26, 1, 26*2)
	VLDR(27, 1, 27*2)
	VLDR(28, 1, 28*2)
	VLDR(29, 1, 29*2)
	VLDR(30, 1, 30*2)
	VLDR(31, 1, 31*2)
fprestx:
	RET
