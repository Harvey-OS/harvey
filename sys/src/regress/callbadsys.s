/* because this moves RARG to ax you can call linux
 * or plan 9 system calls
 */
	TEXT	callbadsys+0(SB),0,$0
	MOVQ RARG, a0+0(FP)
	MOVQ	a1+8(FP),DI
	MOVQ	a2+16(FP),SI
	MOVQ	a3+24(FP),DX
	MOVQ	a4+32(FP),R10
	MOVQ	a5+40(FP),R8
	MOVQ	a6+48(FP),R9
	MOVQ RARG,AX
	SYSCALL
	RET	,
	END	,
