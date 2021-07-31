#include "mem.h"
#include "machine.h"

#define PHYSADDR(v)	(v)
#define VIRTADDR(p)	(p)

#define PPAGE0(offset)	(RAMBASE+(offset))

#define WAIT		NOP; NOP; NOP
#define FLUSH		FLUSHP; FLUSHI

TEXT _startup(SB), $0
	FLUSH
	MOV	$(PM|PE|IE|SE|PX|KL), %CONFIG
	MOV	$0, %PSW
	MOV	$PHYSADDR(MACHADDR+ISPOFFSET), %ISP
	MOV	%ISP, %SHAD
	WAIT

	ENTER	R-16
_stbinit:
	MOV	$PHYSADDR(KSTBADDR), R0
	MOV	R0, %STB				/* physical */

	SUB3	$16, %ISP				/* clear two pages */
l0:
	CMPEQ	R4, R0
	DQM	$0, *R4
	SUB	$16, R4
	JMPF	l0

	ADD3	$(SEGNUM(KZERO)<<2), R0			/* kernel STE */
	MOV	$NPSTE(RAMBASE, 1024, STE_C|STE_W|STE_V), *R4

	ADD3	$(SEGNUM(VROMBASE)<<2), R0		/* ROM STE */
	MOV	$NPSTE(PROMBASE, 128, STE_U|STE_V), *R4

	ADD3	$(SEGNUM(IOBASE)<<2), R0		/* I/O ACK STE */
	MOV	$NPSTE(IOBASE, 4, STE_U|STE_W|STE_V), *R4	/* IIC hack */

	ADD3	$(SEGNUM(IOBASE+0x08000000)<<2), R0	/* I/O NACK STE */
	MOV	$NPSTE(IOBASE+0x08000000, 4, STE_W|STE_V), *R4

	MOV	$(MACHADDR+SPOFFSET), R0
	MOV	R0, R4
	MOV	$_virtual(SB), R8
	MOV	$(PSW_VP|PSW_UL|PSW_S), R12
	FLUSH
	CRET

TEXT _virtual(SB), $16
	MOV	$(MACHADDR+ISPOFFSET), %ISP	/* virtual */

	MOV	$VBADDR, R4			/* initialise vector base */
	MOV	R4, %VB

	MOV	$_vector0(SB), *R4		/* syscall */
	ADD	$4, R4
	MOV	$_vector1(SB), *R4		/* exception */
	ADD	$4, R4
	MOV	$_vector2(SB), *R4		/* niladic unimp */
	ADD	$4, R4
	MOV	$_vector3(SB), *R4		/* unimplemented  */
	ADD	$4, R4
	MOV	$_vector4(SB), *R4		/* NMI */
	ADD	$4, R4
	MOV	$_vector5(SB), *R4		/* interrupt 1 */
	ADD	$4, R4
	MOV	$_vector6(SB), *R4		/* interrupt 2 */
	ADD	$4, R4
	MOV	$_vector7(SB), *R4		/* interrupt 3 */
	ADD	$4, R4
	MOV	$_vector8(SB), *R4		/* interrupt 4 */
	ADD	$4, R4
	MOV	$_vector9(SB), *R4		/* interrupt 5 */
	ADD	$4, R4
	MOV	$_vectorA(SB), *R4		/* interrupt 6 */
	ADD	$4, R4
	MOV	$_vectorB(SB), *R4		/* timer 1 */
	ADD	$4, R4
	MOV	$_vectorC(SB), *R4		/* timer 2 */
	ADD	$4, R4
	MOV	$_vectorD(SB), *R4		/* FP exception */

	MOV	*$PPAGE0(0x74), *$PPAGE0(0x08)	/* fpi */
	MOV	*$PPAGE0(0x74), *$PPAGE0(0x0C)

	MOV	$MACHADDR, m(SB)		/* initialise m */
	MOV	$MACHADDR, mach0(SB)

	MOV	$edata(SB), R4			/* clear BSS */
_clear:
	CMPHI	R4, $end(SB)
	JMPT	_done
	MOV	$0, *R4
	ADD	$4, R4
	JMP	_clear
_done:
	CALL	main(SB)
	RETURN					/* better not... */

TEXT splhi(SB), $0
	MOV	%PSW, R4
	AND	$~PSW_IPL, %PSW
	RETURN

TEXT spllo(SB), $0
	MOV	%PSW, R4
	OR	$PSW_IPL, %PSW
	RETURN

TEXT splx(SB), $0
	MOV	R4, %PSW
TEXT spldone(SB), $0
	RETURN

TEXT setlabel(SB), $0
	MOV	%ISP, *R4
	ADD	$4, R4
	MOV	%SP, *R4
	ADD	$4, R4
	MOV	%MSP, *R4
	ADD	$4, R4
	MOV	R0, *R4
	ADD	$4, R4
	MOV	%PSW, *R4
	MOV	$0, R4
	RETURN

TEXT gotolabel(SB), $SCSIZE		/* gotolabel(Label *l) */
	MOV	l+0(FP), R0		/* l */
	MOV	$(PSW_VP|PSW_UL|PSW_S), %PSW	/* no surprises */

	SUB3	$16, *R0		/* create frame for CRET */
	MOV	R4, %ISP
	ADD	$4, R0
	MOV	*R0, R16		/* SP */
	ADD	$4, R0
	MOV	*R0, R20		/* MSP */
	ADD	$4, R0
	MOV	*R0, R24		/* PC */
	ADD	$4, R0
	MOV	*R0, R28		/* PSW */
	DQM	R16, *R4		/* load CRET frame */

	ADD	$4, R16
	MOV	$1, *R16		/* return value */

	FLUSHI
	CRET

TEXT tas(SB), $0
	ORI	$1, *R4
	RETURN

TEXT gettimer1(SB), $0
	MOV	%TIMER1, R4
	MOV	$0, %TIMER1
	RETURN

TEXT mmuflushpte(SB), $0
	FLUSHPTE *R4
	RETURN

TEXT mmuflushtlb(SB), $0
	MOV	%STB, %STB
	RETURN

TEXT mmusetstb(SB), $0
	OR	$STB_C, R4
	MOV	R4, %STB
	RETURN

TEXT flushcpucache(SB), $0
	FLUSH
	RETURN

TEXT _vectorD(SB), $16			/* FP exception */
	MOV	$0x0D, R0
	JMP	_exception

TEXT _vectorC(SB), $16			/* timer 2 interrupt */
	MOV	$0x0C, R0
	JMP	_exception

TEXT _vectorB(SB), $16			/* timer 1 interrupt */
	MOV	$0x0B, R0
	JMP	_exception

TEXT _vectorA(SB), $16			/* interrupt 6 */
	MOV	$0x0A, R0
	JMP	_exception

TEXT _vector9(SB), $16			/* interrupt 5 */
	MOV	$0x09, R0
	JMP	_exception

TEXT _vector8(SB), $16			/* interrupt 4 */
	MOV	$0x08, R0
	JMP	_exception

TEXT _vector7(SB), $16			/* interrupt 3 */
	MOV	$0x07, R0
	JMP	_exception

TEXT _vector6(SB), $16			/* interrupt 2 */
	MOV	$0x06, R0
	JMP	_exception

TEXT _vector5(SB), $16			/* interrupt 1 */
	MOV	$0x05, R0
	JMP	_exception

TEXT _vector4(SB), $16			/* non-maskable interrupt */
	MOV	$0x04, R0
	JMP	_exception

TEXT _vector3(SB), $16			/* unimplemented instruction */
	MOV	$0x03, R0
	JMP	_exception

TEXT _vector2(SB), $16			/* niladic trap */
	MOV	$0x02, R0
	JMP	_exception

TEXT _vector1(SB), $16			/* exception */
	MOV	$0x01, R0
	JMP	_exception		/* no return */

TEXT _vector0(SB), $16			/* syscall */
	MOV	$0x00, R0

_exception:
	MOV	%SP, R16
	SUB3	R16, %MSP
	CMPHI	R4, $SCSIZE
	AND3	$PSW_X, R28		/* was it user mode? */
	MOV	%FAULT, R12
	JMPFY	_mspok
	MOV	R16, %MSP
_mspok:
	CMPEQ	$0, R4
	MOV	R20, R8			/* id */
	MOV	%MSP, R20
	OR	$PSW_S, %PSW		/* switch to SP */
	MOV	%SP, %SHAD
	JMPFY	_wasuser

_waskernel:
	ENTER	R-16
	MOV	%ISP, R4
	MOV	*R4, R8			/* HACK -- tidy later */
	SHL	$2, R8			/* HACK -- tidy later */
	ADD	$_vectors(SB), R8	/* HACK -- tidy later */
	MOV	*R8, R8			/* HACK -- tidy later */
	CALL	*R8			/* HACK -- tidy later */
	ADD	$16, %ISP
	ADD3	$4, %ISP
	MOV	*R4, R4			/* this doesn't quite cut it */
	CATCH	*R4
	POPN	R16
	KRET

_wasuser:
	ENTER	$(USERADDR+SPOFFSET-16)
	CATCH	R16
	MOV	%ISP, R4
	MOV	*R4, R8			/* HACK -- tidy later */
	SHL	$2, R8			/* HACK -- tidy later */
	ADD	$_vectors(SB), R8	/* HACK -- tidy later */
	MOV	*R8, R8			/* HACK -- tidy later */
	CALL	*R8			/* HACK -- tidy later */

TEXT touser(SB), $0
	ADD	$16, %ISP

	ADD3	$8, %ISP		/* Hobbit Mask3 bug */
	CMPEQ	*R4, $_cret(SB)
	JMPTY	__cret
	MOV	*R4, *$(USERADDR+SPOFFSET)

TEXT _cret(SB), $0			/* Hobbit Mask3 bug */
__cret:
	CRET

GLOBL _vectors(SB), $VBSIZE
	DATA	_vectors+0x00(SB).W, $syscall(SB)	/* syscall */
	DATA	_vectors+0x04(SB).W, $trap(SB)		/* exception */
	DATA	_vectors+0x08(SB).W, $trap(SB)		/* niladic trap */
	DATA	_vectors+0x0C(SB).W, $trap(SB)		/* unimplemented instruction */
	DATA	_vectors+0x10(SB).W, $trap(SB)		/* non-maskable interrupt */
	DATA	_vectors+0x14(SB).W, $trap(SB)		/* interrupt 1 */
	DATA	_vectors+0x18(SB).W, $trap(SB)		/* interrupt 2 */
	DATA	_vectors+0x1C(SB).W, $trap(SB)		/* interrupt 3 */
	DATA	_vectors+0x20(SB).W, $trap(SB)		/* interrupt 4 */
	DATA	_vectors+0x24(SB).W, $trap(SB)		/* interrupt 5 */
	DATA	_vectors+0x28(SB).W, $interrupt(SB)	/* interrupt 6 */
	DATA	_vectors+0x2C(SB).W, $trap(SB)		/* timer 1 interrupt */
	DATA	_vectors+0x30(SB).W, $trap(SB)		/* timer 2 interrupt */
	DATA	_vectors+0x34(SB).W, $trap(SB)		/* FP exception */

TEXT softreset(SB), $0
_softreset:
	MOV	%CONFIG, *$(RAMBASE+0x004C)
	MOV	%PSW, *$(RAMBASE+0x0050)
	MOV	R0, *$(RAMBASE+0x0068)
	MOV	R16, *$(RAMBASE+0x006C)
	MOV	$PSW_VP, %PSW
	MOV	%ISP, %SHAD
	ENTER	R-16
	MOV	$PROMBASE, R8		/* PC */
	MOV	$0, R12			/* PSW */
	MOV	$0, %CONFIG
	FLUSH
	KRET

/*
 * ulong getcpureg(int);
 * Return the current value of the CPU register
 * given as argument.
 */
TEXT getcpureg(SB), $0
	CMPEQ	$PSW, R4
	JMPFN	_timer1
	MOV	%PSW, R4
	RETURN

_timer1:
	CMPEQ	$TIMER1, R4
	JMPFY	_timer2
	MOV	%TIMER1, R4
	RETURN

_timer2:
	CMPEQ	$TIMER2, R4
	JMPFY	_msp
	MOV	%TIMER2, R4
	RETURN

_msp:
	CMPEQ	$MSP, R4
	JMPFY	_isp
	MOV	%MSP, R4
	RETURN

_isp:
	CMPEQ	$ISP, R4
	JMPFY	_sp
	MOV	%ISP, R4
	RETURN

_sp:
	CMPEQ	$SP, R4
	JMPFY	_config
	MOV	%SP, R4
	RETURN

_config:
	CMPEQ	$CONFIG, R4
	JMPFY	_vb
	MOV	%CONFIG, R4
	RETURN

_shad:
	CMPEQ	$SHAD, R4
	JMPFY	_vb
	MOV	%SHAD, R4
	RETURN

_vb:
	CMPEQ	$VB, R4
	JMPFY	_stb
	MOV	%VB, R4
	RETURN

_stb:
	CMPEQ	$STB, R4
	JMPFY	_fault
	MOV	%STB, R4
	RETURN

_fault:
	CMPEQ	$FAULT, R4
	JMPFY	_id
	MOV	%FAULT, R4
	RETURN

_id:
	CMPEQ	$ID, R4
	JMPFY	_nota
	MOV	%ID, R4
	RETURN

_nota:
	MOV	$0, R4
	RETURN
