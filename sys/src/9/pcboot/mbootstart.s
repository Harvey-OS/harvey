#include "mem.h"
#include "/sys/src/boot/pc/x16.h"

/*
 * Must be 4-byte aligned & within 8K of the image's start.
 */
TEXT _nop(SB), $0
	NOP
	NOP
	NOP
#include "mboot.s"
