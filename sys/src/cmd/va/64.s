#define	CONST(x,r) MOVW $((x)&0xffff0000), r; OR  $((x)&0xffff), r
#define	VCONST(x,r) MOVW $(((x)>>32)&0xffffffff), r; \
		SLLV	$32, r; \
		OR	$((x)&0xffffffff), r

	NOSCHED

TEXT _main(SB), $0
	MOVW	$setR30(SB), R30

//	MOVVL	$0x123456789abcdef0, R1
	VCONST(0x123456789abcdef0, R1)
	RET
