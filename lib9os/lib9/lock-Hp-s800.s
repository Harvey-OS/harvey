;	
;	/*
;	 * To get lock routine, compile this into a .s, then SUBSTITUTE
;	 * a LOAD AND CLEAR WORD instruction for the load and store of
;	 * l->key.
;	 *
;	 */
;	typedef struct Lock {
;		int	key;
;	} Lock;
;	
;	int
;	mutexlock(Lock *l)
;	{
;		int key;
;	
;		key = l->key;
;		l->key = 0;
;		return key != 0;
;	}

	.LEVEL	1.1

	.SPACE	$TEXT$,SORT=8
	.SUBSPA	$CODE$,QUAD=0,ALIGN=8,ACCESS=0x2c,CODE_ONLY,SORT=24
mutexlock
	.PROC
	.CALLINFO FRAME=0,ARGS_SAVED
	.ENTRY
; SUBSTITUTED	LDW	0(%r26),%r31
; SUBSTITUTED	STWS	%r0,0(%r26)
	LDCWS	0(%r26),%r31	; SUBSTITUTED
	COMICLR,=	0,%r31,%r28
	LDI	1,%r28
	.EXIT
	BV,N	%r0(%r2)
	.PROCEND
	.end
