/*
 * version of main9.s for use in Linux64 test environment with 7load
 */
#define NPRIVATES	16

/*
 * assume abi expects R8 on to be preserved
 */

TEXT	_main(SB), 1, $-1
	MOV	R28, R6
	MOV	$setSB(SB), R28
	MOV	SP, R7
	MOV	R7, _origSP(SB)
	MOV	R8, _origR8(SB)
	MOV	R9, _origR9(SB)
	MOV	R10, _origR10(SB)
	MOV	R11, _origR11(SB)
	MOV	R12, _origR12(SB)
	MOV	R13, _origR13(SB)
	MOV	R14, _origR14(SB)
	MOV	R15, _origR15(SB)
	MOV	R16, _origR16(SB)
	MOV	R17, _origR17(SB)
	MOV	R18, _origR18(SB)
	MOV	R19, _origR19(SB)
	MOV	R20, _origR20(SB)
	MOV	R21, _origR21(SB)
	MOV	R22, _origR22(SB)
	MOV	R23, _origR23(SB)
	MOV	R24, _origR24(SB)
	MOV	R25, _origR25(SB)
	MOV	R26, _origR26(SB)
	MOV	R27, _origR27(SB)
	MOV	R6, _origR28(SB)
	MOV	R29, _origR29(SB)

	MOV	SP, R4
	MOV	R4, _tos(SB)
	SUB	$(16*1024), SP, R4
	MOV	R4, _privates(SB)
	MOVW	$NPRIVATES, R5
	MOVW	R5, _nprivates(SB)
	SUB	$32, R4
	MOV	R4, SP
	MOV	R0, 8(SP)	/* argc (doesn't need to be stored) */
	MOV	R1, 16(SP)	/* argv */
	MOV	R2, _svc(SB)
	BL	main(SB)

loop:
	MOV	$_exits<>(SB), RARG
	BL	_exits(SB)
	B	loop

TEXT _callsys(SB), $0
	MOV	SP, R7
	MOV	R7, _newSP(SB)
	MOV	_origSP(SB), R7
	MOV	R7, SP
	MOV	_svc(SB), R7
	MOV	_sysargs(SB), R0
	MOV	_sysargs+8(SB), R1
	MOV	_sysargs+16(SB), R2
	MOV	_sysargs+24(SB), R3
	MOV	_origR8(SB), R8
	MOV	_origR9(SB), R9
	MOV	_origR10(SB), R10
	MOV	_origR11(SB), R11
	MOV	_origR12(SB), R12
	MOV	_origR13(SB), R13
	MOV	_origR14(SB), R14
	MOV	_origR15(SB), R15
	MOV	_origR16(SB), R16
	MOV	_origR17(SB), R17
	MOV	_origR18(SB), R18
	MOV	_origR19(SB), R19
	MOV	_origR20(SB), R20
	MOV	_origR21(SB), R21
	MOV	_origR22(SB), R22
	MOV	_origR23(SB), R23
	MOV	_origR24(SB), R24
	MOV	_origR25(SB), R25
	MOV	_origR26(SB), R26
	MOV	_origR27(SB), R27
	MOV	_origR29(SB), R29
	MOV	_origR28(SB), R28
	BL	(R7)
	/* result in R0 */
	MOV	$setSB(SB), R28
	MOV	_newSP(SB), R4
	MOV	R4, SP
	RETURN

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5

GLOBL	_origR8(SB), $8
GLOBL	_origR9(SB), $8
GLOBL	_origR10(SB), $8
GLOBL	_origR11(SB), $8
GLOBL	_origR12(SB), $8
GLOBL	_origR13(SB), $8
GLOBL	_origR14(SB), $8
GLOBL	_origR15(SB), $8
GLOBL	_origR16(SB), $8
GLOBL	_origR17(SB), $8
GLOBL	_origR18(SB), $8
GLOBL	_origR19(SB), $8
GLOBL	_origR20(SB), $8
GLOBL	_origR21(SB), $8
GLOBL	_origR22(SB), $8
GLOBL	_origR23(SB), $8
GLOBL	_origR24(SB), $8
GLOBL	_origR25(SB), $8
GLOBL	_origR26(SB), $8
GLOBL	_origR27(SB), $8
GLOBL	_origR28(SB), $8
GLOBL	_origR29(SB), $8
GLOBL	_origSP(SB), $8
GLOBL	_newSP(SB), $8
GLOBL	_sysargs(SB), $64
GLOBL	_svc(SB), $8
