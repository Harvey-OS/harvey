/*
 File: getcallerpc-Darwin-power.s

 getcallerpc() is passed a pointer

 Plan 9 asm:
 
 TEXT	getcallerpc(SB), $-4
	MOVW	0(R1), R3
	RETURN
 
 */

#import <architecture/ppc/asm_help.h>

.text
LABEL(_getcallerpc)
	mflr r3
	blr
