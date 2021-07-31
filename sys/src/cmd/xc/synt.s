bit= 0 et= 0 synt+0(SB)
bit= 1 et=11 band+0(FP)
bit= 2 et= 5 j-8(SP)
bit= 3 et= 5 num_bands+4(FP)
bit= 4 et= 5 i-4(SP)
bit= 5 et= 5 band_end-20(SP)
bit= 6 et= 5 nn-16(SP)
bit= 7 et=11 mask+8(FP)
bit= 8 et=11 rr+12(FP)
bit= 9 et=11 r-32(SP)
bit=10 et=11 rl+16(FP)
bit=11 et=11 l-36(SP)
bit=12 et= 5 k-12(SP)
bit=13 et= 9 vrr-24(SP)
bit=14 et= 9 vrl-28(SP)

test/synt.c:2 synt+0(SB)

looping structure:
1:	TEXT	synt+0(SB),$36	 u1=synt
1:	MOVW	R3,band+0(FP)	 st=band
1:	MOVW	$0,j-8(SP)	 st=j
1:	MOVW	num_bands+4(FP),R16 u1=num_bands
1:	MOVW	R16,i-4(SP)	 st=i
1:	JMP	,-73(PC)
4:	JMP	,-76(PC)
1:	JMP	,-3(PC)
4:	MOVW	i-4(SP),R19	 u1=i
4:	ADD	$-1,R19
4:	MOVW	R19,i-4(SP)	 st=i
4:	MOVW	i-4(SP),R16	 u1=i
4:	CMP	R16,$0
4:	BRA	EQ,,-77(PC)
4:	MOVW	band+0(FP),R4	 u1=band
4:	ADD	$4,R4,R5
4:	MOVW	R5,band+0(FP)	 st=band
4:	MOVW	0(R4),R19
4:	MOVW	R19,band_end-20(SP) st=band_end
4:	MOVW	band_end-20(SP),R17 u1=band_end
4:	MOVW	j-8(SP),R19	 u1=j
4:	SUB	R19,R17
4:	MOVW	R17,R16
4:	MOVW	R16,nn-16(SP)	 st=nn
4:	MOVW	mask+8(FP),R4	 u1=mask
4:	ADD	$4,R4,R5
4:	MOVW	R5,mask+8(FP)	 st=mask
4:	MOVW	0(R4),R17
4:	CMP	R17,$0
4:	BRA	EQ,,-16(PC)
4:	MOVW	rr+12(FP),R4	 u1=rr
4:	MOVW	R4,r-32(SP)	 st=r
4:	MOVW	rl+16(FP),R4	 u1=rl
4:	MOVW	R4,l-36(SP)	 st=l
4:	MOVW	$0,k-12(SP)	 st=k
4:	JMP	,-43(PC)
4:	JMP	,-46(PC)
4:	JMP	,-16(PC)
4:	MOVW	k-12(SP),R19	 u1=k
4:	ADD	$1,R19
4:	MOVW	R19,k-12(SP)	 st=k
7:	MOVW	k-12(SP),R16	 u1=k
7:	MOVW	nn-16(SP),R19	 u1=nn
7:	CMP	R16,R19
7:	BRA	GE,,-47(PC)
4:	MOVW	r-32(SP),R4	 u1=r
4:	FMOVF	0(R4),F0
4:	FMOVF	F0,vrr-24(SP)	 st=vrr
4:	MOVW	l-36(SP),R4	 u1=l
4:	FMOVF	0(R4),F0
4:	FMOVF	F0,vrl-28(SP)	 st=vrl
4:	MOVW	r-32(SP),R4	 u1=r
4:	FMOVF	vrr-24(SP),F0	 u1=vrr
4:	FMOVF	vrl-28(SP),F1	 u1=vrl
4:	FADD	F1,F0,F0
4:	FMOVF	F0,0(R4)
4:	MOVW	l-36(SP),R4	 u1=l
4:	FMOVF	vrr-24(SP),F0	 u1=vrr
4:	FMOVF	vrl-28(SP),F1	 u1=vrl
4:	FSUB	F1,F0,F0
4:	FMOVF	F0,0(R4)
4:	MOVW	r-32(SP),R4	 u1=r
4:	ADD	$4,R4
4:	MOVW	R4,r-32(SP)	 st=r
4:	MOVW	l-36(SP),R4	 u1=l
4:	ADD	$4,R4
4:	MOVW	R4,l-36(SP)	 st=l
4:	JMP	,-48(PC)
4:	MOVW	nn-16(SP),R4	 u1=nn
4:	SLL	$2,R4
4:	MOVW	rr+12(FP),R5	 u1=rr
4:	ADD	R4,R5,R4
4:	MOVW	R4,rr+12(FP)	 st=rr
4:	MOVW	nn-16(SP),R4	 u1=nn
4:	SLL	$2,R4
4:	MOVW	rl+16(FP),R5	 u1=rl
4:	ADD	R4,R5,R4
4:	MOVW	R4,rl+16(FP)	 st=rl
4:	MOVW	band_end-20(SP),R17 u1=band_end
4:	MOVW	R17,j-8(SP)	 st=j
4:	JMP	,-78(PC)
1:	NOP	,R3
1:	NOP	,F0
1:	RETURN	,

prop structure:
	TEXT	synt+0(SB),$36
	set = ; rah = num_bands mask rr rl; cal = synt

1	TEXT	synt+0(SB),$36	ld num_bands $-5 $0
1	MOVW	num_bands+4(FP),R16u1 num_bands $0 $0
test/synt.c:2$0: num_bands

1	TEXT	synt+0(SB),$36	ld mask $-5 $0
4	MOVW	R5,mask+8(FP)	se mask $15 $0
4	MOVW	mask+8(FP),R4	u1 mask $35 $0

1	TEXT	synt+0(SB),$36	ld rr $-5 $0
4	MOVW	R4,rr+12(FP)	se rr $15 $0
4	MOVW	rr+12(FP),R4	u1 rr $35 $0
4	MOVW	rr+12(FP),R5	u1 rr $55 $0

1	TEXT	synt+0(SB),$36	ld rl $-5 $0
4	MOVW	R4,rl+16(FP)	se rl $15 $0
4	MOVW	rl+16(FP),R4	u1 rl $35 $0
4	MOVW	rl+16(FP),R5	u1 rl $55 $0
	MOVW	R3,band+0(FP)
	set = band; rah = band num_bands mask rr rl; cal = synt

1	MOVW	R3,band+0(FP)	se band $5 $0
4	MOVW	R5,band+0(FP)	se band $25 $0
4	MOVW	band+0(FP),R4	u1 band $45 $0
	MOVW	$0,j-8(SP)
	set = j; rah = band j num_bands mask rr rl; cal = synt

1	MOVW	$0,j-8(SP)	se j $5 $0
4	MOVW	R17,j-8(SP)	se j $25 $0
4	MOVW	j-8(SP),R19	u1 j $45 $0
	MOVW	num_bands+4(FP),R16
	set = ; rah = band j mask rr rl; cal = synt
	MOVW	R16,i-4(SP)
	set = i; rah = band j i mask rr rl; cal = synt

1	MOVW	R16,i-4(SP)	se i $5 $0
4	MOVW	R19,i-4(SP)	se i $25 $0
4	MOVW	i-4(SP),R16	u1 i $45 $0
4	MOVW	i-4(SP),R19	u1 i $65 $0
	JMP	,-73(PC)
	set = ; rah = band j i mask rr rl; cal = synt
	JMP	,-76(PC)
	set = ; rah = band j i mask rr rl; cal = synt
	JMP	,-3(PC)
	set = ; rah = ; cal = synt
	MOVW	i-4(SP),R19
	set = ; rah = band j mask rr rl; cal = synt
	ADD	$-1,R19
	set = ; rah = band j mask rr rl; cal = synt
	MOVW	R19,i-4(SP)
	set = i; rah = band j i mask rr rl; cal = synt
	MOVW	i-4(SP),R16
	set = ; rah = band j i mask rr rl; cal = synt
	CMP	R16,$0
	set = ; rah = band j i mask rr rl; cal = synt
	BRA	EQ,,-77(PC)
	set = ; rah = band j i mask rr rl; cal = synt
	MOVW	band+0(FP),R4
	set = ; rah = j i mask rr rl; cal = synt
	ADD	$4,R4,R5
	set = ; rah = j i mask rr rl; cal = synt
	MOVW	R5,band+0(FP)
	set = band; rah = band j i mask rr rl; cal = synt
	MOVW	0(R4),R19
	set = ; rah = band j i mask rr rl; cal = synt
	MOVW	R19,band_end-20(SP)
	set = band_end; rah = band j i band_end mask rr rl; cal = synt

4	MOVW	R19,band_end-20(SP)se band_end $20 $0
4	MOVW	band_end-20(SP),R17u1 band_end $40 $0
4	MOVW	band_end-20(SP),R17u1 band_end $60 $0
	MOVW	band_end-20(SP),R17
	set = ; rah = band j i band_end mask rr rl; cal = synt
	MOVW	j-8(SP),R19
	set = ; rah = band i band_end mask rr rl; cal = synt
	SUB	R19,R17
	set = ; rah = band i band_end mask rr rl; cal = synt
	MOVW	R17,R16
	set = ; rah = band i band_end mask rr rl; cal = synt
	MOVW	R16,nn-16(SP)
	set = nn; rah = band i band_end nn mask rr rl; cal = synt

4	MOVW	R16,nn-16(SP)	se nn $20 $0
4	MOVW	nn-16(SP),R4	u1 nn $40 $0
7	MOVW	nn-16(SP),R19	u1 nn $75 $0
4	MOVW	nn-16(SP),R4	u1 nn $95 $0
	MOVW	mask+8(FP),R4
	set = ; rah = band i band_end nn rr rl; cal = synt
	ADD	$4,R4,R5
	set = ; rah = band i band_end nn rr rl; cal = synt
	MOVW	R5,mask+8(FP)
	set = mask; rah = band i band_end nn mask rr rl; cal = synt
	MOVW	0(R4),R17
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	CMP	R17,$0
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	BRA	EQ,,-16(PC)
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	MOVW	rr+12(FP),R4
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	MOVW	R4,r-32(SP)
	set = r; rah = band i band_end nn mask rr r rl; cal = synt

4	MOVW	R4,r-32(SP)	se r $20 $0
4	MOVW	R4,r-32(SP)	se r $40 $0
4	MOVW	r-32(SP),R4	u1 r $60 $0
4	MOVW	r-32(SP),R4	u1 r $80 $0
4	MOVW	r-32(SP),R4	u1 r $100 $0
	MOVW	rl+16(FP),R4
	set = ; rah = band i band_end nn mask rr r rl; cal = synt
	MOVW	R4,l-36(SP)
	set = l; rah = band i band_end nn mask rr r rl l; cal = synt

4	MOVW	R4,l-36(SP)	se l $20 $0
4	MOVW	R4,l-36(SP)	se l $40 $0
4	MOVW	l-36(SP),R4	u1 l $60 $0
4	MOVW	l-36(SP),R4	u1 l $80 $0
4	MOVW	l-36(SP),R4	u1 l $100 $0
	MOVW	$0,k-12(SP)
	set = k; rah = band i band_end nn mask rr r rl l k; cal = synt

4	MOVW	$0,k-12(SP)	se k $20 $0
4	MOVW	R19,k-12(SP)	se k $40 $0
7	MOVW	k-12(SP),R16	u1 k $75 $0
4	MOVW	k-12(SP),R19	u1 k $95 $0
	JMP	,-43(PC)
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	JMP	,-46(PC)
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	JMP	,-16(PC)
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	MOVW	k-12(SP),R19
	set = ; rah = band i band_end nn mask rr r rl l; cal = synt
	ADD	$1,R19
	set = ; rah = band i band_end nn mask rr r rl l; cal = synt
	MOVW	R19,k-12(SP)
	set = k; rah = band i band_end nn mask rr r rl l k; cal = synt
	MOVW	k-12(SP),R16
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	MOVW	nn-16(SP),R19
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	CMP	R16,R19
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	BRA	GE,,-47(PC)
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	MOVW	r-32(SP),R4
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	FMOVF	0(R4),F0
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	FMOVF	F0,vrr-24(SP)
	set = vrr; rah = band i band_end nn mask rr r rl l k vrr; cal = synt

4	FMOVF	F0,vrr-24(SP)	la vrr $0 $-8
4	FMOVF	F0,vrr-24(SP)	se vrr $20 $4
4	FMOVF	vrr-24(SP),F0	u1 vrr $40 $16
4	FMOVF	vrr-24(SP),F0	u1 vrr $60 $28
	MOVW	l-36(SP),R4
	set = ; rah = band i band_end nn mask rr r rl l k vrr; cal = synt
	FMOVF	0(R4),F0
	set = ; rah = band i band_end nn mask rr r rl l k vrr; cal = synt
	FMOVF	F0,vrl-28(SP)
	set = vrl; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt

4	FMOVF	F0,vrl-28(SP)	la vrl $0 $-8
4	FMOVF	F0,vrl-28(SP)	se vrl $20 $4
4	FMOVF	vrl-28(SP),F1	u1 vrl $40 $16
4	FMOVF	vrl-28(SP),F1	u1 vrl $60 $28
	MOVW	r-32(SP),R4
	set = ; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt
	FMOVF	vrr-24(SP),F0
	set = ; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt
	FMOVF	vrl-28(SP),F1
	set = ; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt
	FADD	F1,F0,F0
	set = ; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt
	FMOVF	F0,0(R4)
	set = ; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt
	MOVW	l-36(SP),R4
	set = ; rah = band i band_end nn mask rr r rl l k vrr vrl; cal = synt
	FMOVF	vrr-24(SP),F0
	set = ; rah = band i band_end nn mask rr r rl l k vrl; cal = synt
	FMOVF	vrl-28(SP),F1
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	FSUB	F1,F0,F0
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	FMOVF	F0,0(R4)
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	MOVW	r-32(SP),R4
	set = ; rah = band i band_end nn mask rr rl l k; cal = synt
	ADD	$4,R4
	set = ; rah = band i band_end nn mask rr rl l k; cal = synt
	MOVW	R4,r-32(SP)
	set = r; rah = band i band_end nn mask rr r rl l k; cal = synt
	MOVW	l-36(SP),R4
	set = ; rah = band i band_end nn mask rr r rl k; cal = synt
	ADD	$4,R4
	set = ; rah = band i band_end nn mask rr r rl k; cal = synt
	MOVW	R4,l-36(SP)
	set = l; rah = band i band_end nn mask rr r rl l k; cal = synt
	JMP	,-48(PC)
	set = ; rah = band i band_end nn mask rr r rl l k; cal = synt
	MOVW	nn-16(SP),R4
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	SLL	$2,R4
	set = ; rah = band i band_end nn mask rr rl; cal = synt
	MOVW	rr+12(FP),R5
	set = ; rah = band i band_end nn mask rl; cal = synt
	ADD	R4,R5,R4
	set = ; rah = band i band_end nn mask rl; cal = synt
	MOVW	R4,rr+12(FP)
	set = rr; rah = band i band_end nn mask rr rl; cal = synt
	MOVW	nn-16(SP),R4
	set = ; rah = band i band_end mask rr rl; cal = synt
	SLL	$2,R4
	set = ; rah = band i band_end mask rr rl; cal = synt
	MOVW	rl+16(FP),R5
	set = ; rah = band i band_end mask rr; cal = synt
	ADD	R4,R5,R4
	set = ; rah = band i band_end mask rr; cal = synt
	MOVW	R4,rl+16(FP)
	set = rl; rah = band i band_end mask rr rl; cal = synt
	MOVW	band_end-20(SP),R17
	set = ; rah = band i mask rr rl; cal = synt
	MOVW	R17,j-8(SP)
	set = j; rah = band j i mask rr rl; cal = synt
	JMP	,-78(PC)
	set = ; rah = band j i mask rr rl; cal = synt
	NOP	,R3
	set = ; rah = ; cal = synt
	NOP	,F0
	set = ; rah = ; cal = synt
	RETURN	,
	set = ; rah = ; cal = 
test/synt.c:13$100 $0 R6: l
	MOVW	R4,l-36(SP)	.c	MOVW	R4,R6
	MOVW	R4,l-36(SP)	.c	MOVW	R4,R6
	MOVW	l-36(SP),R4	.c	MOVW	R6,R4
	MOVW	l-36(SP),R4	.c	MOVW	R6,R4
	MOVW	l-36(SP),R4	.c	MOVW	R6,R4
test/synt.c:12$100 $0 R7: r
	MOVW	R4,r-32(SP)	.c	MOVW	R4,R7
	MOVW	R4,r-32(SP)	.c	MOVW	R4,R7
	MOVW	r-32(SP),R4	.c	MOVW	R7,R4
	MOVW	r-32(SP),R4	.c	MOVW	R7,R4
	MOVW	r-32(SP),R4	.c	MOVW	R7,R4
test/synt.c:14$95 $0 R15: k
	MOVW	$0,k-12(SP)	.c	MOVW	$0,R15
	MOVW	R19,k-12(SP)	.c	MOVW	R19,R15
	MOVW	k-12(SP),R16	.c	MOVW	R15,R16
	MOVW	k-12(SP),R19	.c	MOVW	R15,R19
test/synt.c:10$95 $0 R8: nn
	MOVW	R16,nn-16(SP)	.c	MOVW	R16,R8
	MOVW	nn-16(SP),R4	.c	MOVW	R8,R4
	MOVW	nn-16(SP),R19	.c	MOVW	R8,R19
	MOVW	nn-16(SP),R4	.c	MOVW	R8,R4
test/synt.c:8$65 $0 R9: i
	MOVW	R16,i-4(SP)	.c	MOVW	R16,R9
	MOVW	R19,i-4(SP)	.c	MOVW	R19,R9
	MOVW	i-4(SP),R16	.c	MOVW	R9,R16
	MOVW	i-4(SP),R19	.c	MOVW	R9,R19
test/synt.c:16$60 $28 F2: vrl
	FMOVF	F0,vrl-28(SP)	.c	FMOVF	F0,F2
	FMOVF	vrl-28(SP),F1	.c	FMOVF	F2,F1
	FMOVF	vrl-28(SP),F1	.c	FMOVF	F2,F1
test/synt.c:15$60 $28 F3: vrr
	FMOVF	F0,vrr-24(SP)	.c	FMOVF	F0,F3
	FMOVF	vrr-24(SP),F0	.c	FMOVF	F3,F0
	FMOVF	vrr-24(SP),F0	.c	FMOVF	F3,F0
test/synt.c:9$60 $0 R10: band_end
	MOVW	R19,band_end-20(SP).c	MOVW	R19,R10
	MOVW	band_end-20(SP),R17.c	MOVW	R10,R17
	MOVW	band_end-20(SP),R17.c	MOVW	R10,R17
test/synt.c:2$55 $0 R11: rl
	TEXT	synt+0(SB),$36	.a	MOVW	rl+16(FP),R11
	MOVW	R4,rl+16(FP)	.c	MOVW	R4,R11
	MOVW	rl+16(FP),R4	.c	MOVW	R11,R4
	MOVW	rl+16(FP),R5	.c	MOVW	R11,R5
test/synt.c:2$55 $0 R12: rr
	TEXT	synt+0(SB),$36	.a	MOVW	rr+12(FP),R12
	MOVW	R4,rr+12(FP)	.c	MOVW	R4,R12
	MOVW	rr+12(FP),R4	.c	MOVW	R12,R4
	MOVW	rr+12(FP),R5	.c	MOVW	R12,R5
test/synt.c:7$45 $0 R15: j
	MOVW	$0,j-8(SP)	.c	MOVW	$0,R15
	MOVW	R17,j-8(SP)	.c	MOVW	R17,R15
	MOVW	j-8(SP),R19	.c	MOVW	R15,R19
test/synt.c:2$45 $0 R13: band
	MOVW	R3,band+0(FP)	.c	MOVW	R3,R13
	MOVW	R5,band+0(FP)	.c	MOVW	R5,R13
	MOVW	band+0(FP),R4	.c	MOVW	R13,R4
test/synt.c:2$35 $0 R0: mask
addrs: synt
	TEXT	synt+0(SB),$36
	MOVW	rr+12(FP),R12
	MOVW	rl+16(FP),R11
	MOVW	R3,R13
	MOVW	$0,R15
	MOVW	num_bands+4(FP),R16
	MOVW	R16,R9
	JMP	,6(PC)
	JMP	,2(PC)
	JMP	,74(PC)
	MOVW	R9,R19
	ADD	$-1,R19
	MOVW	R19,R9
	MOVW	R9,R16
	CMP	R16,$0
	BRA	EQ,,-6(PC)
	MOVW	R13,R4
	ADD	$4,R4,R5
	MOVW	R5,R13
	MOVW	0(R4),R19
	MOVW	R19,R10
	MOVW	R10,R17
	MOVW	R15,R19
	SUB	R19,R17
	MOVW	R17,R16
	MOVW	R16,R8
	MOVW	mask+8(FP),R4
	ADD	$4,R4,R5
	MOVW	R5,mask+8(FP)
	MOVW	0(R4),R17
	CMP	R17,$0
	BRA	EQ,,39(PC)
	MOVW	R12,R4
	MOVW	R4,R7
	MOVW	R11,R4
	MOVW	R4,R6
	MOVW	$0,R15
	JMP	,6(PC)
	JMP	,2(PC)
	JMP	,31(PC)
	MOVW	R15,R19
	ADD	$1,R19
	MOVW	R19,R15
	MOVW	R15,R16
	MOVW	R8,R19
	CMP	R16,R19
	BRA	GE,,-7(PC)
	MOVW	R7,R4
	FMOVF	0(R4),F0
	FMOVF	F0,F3
	MOVW	R6,R4
	FMOVF	0(R4),F0
	FMOVF	F0,F2
	MOVW	R7,R4
	FMOVF	F3,F0
	FMOVF	F2,F1
	FADD	F1,F0,F0
	FMOVF	F0,0(R4)
	MOVW	R6,R4
	FMOVF	F3,F0
	FMOVF	F2,F1
	FSUB	F1,F0,F0
	FMOVF	F0,0(R4)
	MOVW	R7,R4
	ADD	$4,R4
	MOVW	R4,R7
	MOVW	R6,R4
	ADD	$4,R4
	MOVW	R4,R6
	JMP	,-31(PC)
	MOVW	R8,R4
	SLL	$2,R4
	MOVW	R12,R5
	ADD	R4,R5,R4
	MOVW	R4,R12
	MOVW	R8,R4
	SLL	$2,R4
	MOVW	R11,R5
	ADD	R4,R5,R4
	MOVW	R4,R11
	MOVW	R10,R17
	MOVW	R17,R15
	JMP	,-74(PC)
	NOP	,R3
	NOP	,F0
	RETURN	,
	END	,
