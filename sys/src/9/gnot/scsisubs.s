#include "mem.h"

TEXT	spl2(SB), $0

	MOVL	$0, R0
	MOVW	SR, R0
	MOVW	$(SUPER|SPL(2)), SR
	RTS

TEXT	scsirecv(SB), $0
	MOVL	$0x40300033, A0	/* data		*/
	MOVL	$0x40300032, A1	/* status	*/
	MOVL	p+0(FP), A2
	BTST	$1, (A1)
	BEQ	rout
rloop:
	MOVB	(A0), (A2)+
	BTST	$1, (A1)
	BEQ	rout

	MOVB	(A0), (A2)+
	BTST	$1, (A1)
	BEQ	rout

	MOVB	(A0), (A2)+
	BTST	$1, (A1)
	BEQ	rout

	MOVB	(A0), (A2)+
	BTST	$1, (A1)
	BNE	rloop
rout:
	MOVL	A2, R0
	RTS

TEXT	scsixmit(SB), $0
	MOVL	$0x40300033, A0	/* data		*/
	MOVL	$0x40300032, A1	/* status	*/
	MOVL	p+0(FP), A2
	BTST	$1, (A1)
	BEQ	xout
xloop:
	MOVB	(A2)+, (A0)
	BTST	$1, (A1)
	BEQ	xout

	MOVB	(A2)+, (A0)
	BTST	$1, (A1)
	BEQ	xout

	MOVB	(A2)+, (A0)
	BTST	$1, (A1)
	BEQ	xout

	MOVB	(A2)+, (A0)
	BTST	$1, (A1)
	BNE	xloop
xout:
	MOVL	A2, R0
	RTS
