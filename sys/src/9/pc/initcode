#include "/sys/src/libc/9syscall/sys.h"

TEXT	main(SB),$0

	/*
	 *  exec("/boot", bootv)
	 */
	LEAL	4(SP),AX
	PUSHL	AX
	LEAL	boot(SB),AX
	PUSHL	AX
	PUSHL	$0
	MOVL	$EXEC,AX
	INT	$64

	/*
	 *  should never get here
	 */
here:
	JMP	here

GLOBL	boot+0(SB),$6
DATA	boot+0(SB)/5,$"/boot"
