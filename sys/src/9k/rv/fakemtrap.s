/*
 * Fake trap catcher for machine mode of RV64.
 */
#include "riscv64l.h"

TEXT mtrap(SB), 1, $-4
	MRET
