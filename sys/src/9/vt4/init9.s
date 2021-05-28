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
	MOVW	$setSB(SB), R2
	MOVW	$0, R0
	SUB	$16,R1				/* make a frame */

	/*
	 * Argv0 is already passed to in R3 so it is already the first arg.
	 * Copy argv0 into the stack and push its address as the second arg.
	 */
	MOVW	R3, 0x14(R1)
	ADD	$0x14, R1, R6
	MOVW	R6, 0x8(R1)

	BL	startboot(SB)

loop:
	BR	loop				/* should never get here */
