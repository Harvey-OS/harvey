/*
 * This is the same as the C programme:
 *
 *	void
 *	main(char* argv0)
 *	{
 *		startboot(argv0, &argv0);
 *	}
 *
 * It is in assembler because SB needs to be
 * set and doing this in C drags in too many
 * other routines.
 */
TEXT main(SB), 1, $8
	MOVW	$setR12(SB), R12		/* load the SB */
	MOVW	$boot(SB), R0

	ADD	$12, R13, R1			/* pointer to 0(FP) */

	MOVW	R0, 4(R13)			/* pass argc, argv */
	MOVW	R1, 8(R13)

	BL	startboot(SB)
_loop:
	B	_loop
