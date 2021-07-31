/*
 * typedef long jmp_buf[2];
 *
 * #define SP	0
 * #define PC	1
 */
TEXT setjmp(SB), 1, $0		/* setjmp(env) */
	MOVA	R0, *env+0(FP)	/* SP */
	ADD	$4, env+0(FP)
	MOV	R0, *env+0(FP)	/* PC */
	MOV	$0, R4
	RETURN

TEXT longjmp(SB), 1, $16		/* longjmp(env, val) */
	CMPEQ	$0, val+4(FP)
	AND3	$~15, *env+0(FP)/* R4 = env[SP] & ~0x0F */
	JMPFY	_valok
	MOV	$1, val+4(FP)
_valok:
	ADD	$4, env+0(FP)	/* env[PC] */
	MOV	*env+0(FP), *R4	/* set new PC */
	ADD	$4, R4		/* point to R4 in new stack */
	MOV	val+4(FP), *R4	/* set return value */
	NOP			/* Mask 1 bug */
	NOP			/* Mask 1 bug */
	ENTER	*R4		/* restore SP/MSP */
	CATCH	R32		/* give a little stack back */
	RETURN	R0
