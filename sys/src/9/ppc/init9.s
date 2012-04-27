/* this is the same as a c program:
 * main(char *argv0){
 *	startboot(argv0, &argv0);
 * }
 *
 * it is in asm because we need to set the SB before
 * doing it and the only way to do this in c drags in
 * too many other routines.
 */

TEXT	_main(SB),$8

	MOVW	$setSB(SB), R2

	/* make a frame */
	SUB	$16,R1

	/* argv0 is already passed to us in R3 so it is already the first arg */

	/* copy argv0 into the stack and push its address as the second arg */
	MOVW	R3,0x14(R1)
	ADD	$0x14,R1,R6
	MOVW	R6,0x8(R1)

	BL	startboot(SB)

	/* should never get here */
loop:
	BR	loop
