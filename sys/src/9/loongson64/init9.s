#include "spim64.s"

TEXT	_main(SB), $16
	MOVV	$setR30(SB), R30

	MOVV	$boot(SB), R1
	ADDVU	$24, R29, R2	/* get a pointer to 0(FP) */
	MOVV	R1, 8(R29)
	MOVV	R2, 16(R29)

	JMP		startboot(SB)
