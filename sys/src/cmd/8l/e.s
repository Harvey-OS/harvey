TEXT	main(SB),$0

	LONG	$main(SB)
	LONG	$gorp(SB)

	MOVL	(AX), GDTR	/* lgtd */
	MOVL	(AX), IDTR	/* lidt */
	MOVL	(AX), SP:SS	/* load full pointer */
	MOVW	AX, LDTR	/* lldt */
	MOVW	AX, MSW		/* lmsw */
	MOVW	AX, TASK	/* ltr */
	MOVW	DS, AX		/* move data, segments */
	MOVL	CR0, AX		/* move to/from special */
	MOVL	GDTR, (AX)	/* sgdt */
	MOVL	IDTR, (AX)	/* sidt */
	MOVW	LDTR, AX	/* sldt */
	SHLL	$16, 10(SP):AX	/* double shift */
	MOVW	MSW, AX		/* smsw */
	MOVW	TASK, AX	/* str */

	RET

GLOBL	gorp(SB), $4
