t = 97
u = 98
v = 99
Q = 131
link = 64

/*
 * takes R(t)||Q ÷ R(u)
 *
 * returns quo in Q
 * returns rem in R(t)
 */
TEXT	_divul_(SB), $-4
	DSTEP0L	R(t), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPL	R(u), R(t)
	DSTEPLL	R(u), R(t)
	DSTEPRL	R(u), R(t)
	JMP	(R(v))

/*
 * takes Q||f2 ÷ f1
 *
 * returns rem in Q
 * returns quo in t
 */
TEXT	_divul(SB),$-4
	MFSR	R(Q), R(t)		/* pick up high num in R(t) */
	ORL	$0, R0, R(u)		/* put low num in Q */
	MTSR	R(u), R(Q)
	MOVL	$0, R(u)
	ORL	R0, R(u), R(u)		/* put den in R(u) */
	CALL	R(v), _divul_(SB)
	MFSR	R(Q), R0
	MTSR	R(t), R(Q)
	JMP	(R(link))

/*
 * takes Q||f2 ÷ f1
 *
 * returns rem in Q
 * returns quo in t
 */
TEXT	_divl(SB),$-4
	MFSR	R(Q), R(t)		/* pick up high num R(t) */
	JMPT	R(t), nnum
	ORL	$0, R0, R(u)		/* put low num in Q */
	MTSR	R(u), R(Q)
	MOVL	$0, R(u)
	ORL	R0, R(u), R(u)		/* put den in R(u) */
	JMPT	R(u), pnumnden
	CALL	R(v), _divul_(SB)
	MFSR	R(Q), R0
	MTSR	R(t), R(Q)
	JMP	(R(link))

pnumnden:	/* botch returns pos rem */
	ISUBL	$0, R(u)
	CALL	R(v), _divul_(SB)
	MFSR	R(Q), R(u)
	MTSR	R(t), R(Q)
	ISUBL	$0, R(u), R0		/* return neg quo */
	JMP	(R(link))

/*
 * numerator is negative.
 * do 64-bit negate.
 */
nnum:
	CPEQL	$0, R0, R(u)		/* is low num 0? */
	JMPT	R(u), nnul1
	ISUBL	$0, R0, R(u)		/* negate R(t)||R0 -> R(t)||R(u) */
	XNORL	$0, R(t)
	JMP	nnul2
nnul1:
	MOVL	$0, R(u)		/* negate R(t)||0 -> R(t)||R(u) */
	ISUBL	$0, R(t)
nnul2:
	MTSR	R(u), R(Q)
	MOVL	$0, R(u)
	ORL	R0, R(u), R(u)		/* put den in R(u) */
	JMPT	R(u), nnumnden
	CALL	R(v), _divul_(SB)
	MFSR	R(Q), R(u)
	MTSR	R(t), R(Q)
	ISUBL	$0, R(u), R0		/* return neg quo */
	JMP	(R(link))

nnumnden:	/* botch returns pos rem */
	ISUBL	$0, R(u)
	CALL	R(v), _divul_(SB)
	MFSR	R(Q), R0
	MTSR	R(t), R(Q)
	JMP	(R(link))
