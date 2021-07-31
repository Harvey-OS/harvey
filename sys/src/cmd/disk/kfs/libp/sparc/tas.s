/*
 * Test and set for the 68020, as a subroutine
 */

TEXT	_tas(SB), $0

	TAS	(R7), R7		/* LDSTUB, thank you ken */
	RETURN
