	TEXT	_sha1block+0(SB),$352

/* x = (wp[off-f] ^ wp[off-8] ^ wp[off-14] ^ wp[off-16]) <<< 1;
 * wp[off] = x;
 * x += A <<< 5;
 * E += 0xca62c1d6 + x;
 * x = FN(B,C,D);
 * E += x;
 * B >>> 2
 */
#define BSWAPDI	BYTE $0x0f; BYTE $0xcf;

#define BODY(off,FN,V,A,B,C,D,E)\
	MOVL (off-64)(BP),DI;\
	XORL (off-56)(BP),DI;\
	XORL (off-32)(BP),DI;\
	XORL (off-12)(BP),DI;\
	ROLL $1,DI;\
	MOVL DI,off(BP);\
	LEAL V(DI)(E*1),E;\
	MOVL A,DI;\
	ROLL $5,DI;\
	ADDL DI,E;\
	FN(B,C,D)\
	ADDL DI,E;\
	RORL $2,B;\

#define BODY0(off,FN,V,A,B,C,D,E)\
	MOVL off(BX),DI;\
	BSWAPDI;\
	MOVL DI,off(BP);\
	LEAL V(DI)(E*1),E;\
	MOVL A,DI;\
	ROLL $5,DI;\
	ADDL DI,E;\
	FN(B,C,D)\
	ADDL DI,E;\
	RORL $2,B;\

/*
 * fn1 = (((C^D)&B)^D);
 */
#define FN1(B,C,D)\
	MOVL C,DI;\
	XORL D,DI;\
	ANDL B,DI;\
	XORL D,DI;\

/*
 * fn24 = B ^ C ^ D
 */
#define FN24(B,C,D)\
	MOVL B,DI;\
	XORL C,DI;\
	XORL D,DI;\

/*
 * fn3 = ((B ^ C) & (D ^= B)) ^ B
 * D ^= B to restore D
 */
#define FN3(B,C,D)\
	MOVL B,DI;\
	XORL C,DI;\
	XORL B,D;\
	ANDL D,DI;\
	XORL B,DI;\
	XORL B,D;\

/*
 * stack offsets
 * void sha1block(uchar *DATA, int LEN, ulong *STATE)
 */
#define	DATA	0
#define	LEN	4
#define	STATE	8

/*
 * stack offsets for locals
 * ulong w[80];
 * uchar *edata;
 * ulong *w15, *w40, *w60, *w80;
 * register local
 * ulong *wp = BP
 * ulong a = eax, b = ebx, c = ecx, d = edx, e = esi
 * ulong tmp = edi
 */
#define WARRAY	(-4-(80*4))
#define TMP1	(-8-(80*4))
#define TMP2	(-12-(80*4))
#define W15	(-16-(80*4))
#define W40	(-20-(80*4))
#define W60	(-24-(80*4))
#define W80	(-28-(80*4))
#define EDATA	(-32-(80*4))

	MOVL data+DATA(FP),AX
	ADDL len+LEN(FP),AX
	MOVL AX,edata+EDATA(SP)

	LEAL aw15+(WARRAY+15*4)(SP),DI
	MOVL DI,w15+W15(SP)
	LEAL aw40+(WARRAY+40*4)(SP),DX
	MOVL DX,w40+W40(SP)
	LEAL aw60+(WARRAY+60*4)(SP),CX
	MOVL CX,w60+W60(SP)
	LEAL aw80+(WARRAY+80*4)(SP),DI
	MOVL DI,w80+W80(SP)

mainloop:
	LEAL warray+WARRAY(SP),BP

	MOVL state+STATE(FP),DI
	MOVL (DI),AX
	MOVL 4(DI),BX
	MOVL BX,tmp1+TMP1(SP)
	MOVL 8(DI),CX
	MOVL 12(DI),DX
	MOVL 16(DI),SI

	MOVL data+DATA(FP),BX

loop1:
	BODY0(0,FN1,0x5a827999,AX,tmp1+TMP1(SP),CX,DX,SI)
	MOVL SI,tmp2+TMP2(SP)
	BODY0(4,FN1,0x5a827999,SI,AX,tmp1+TMP1(SP),CX,DX)
	MOVL tmp1+TMP1(SP),SI
	BODY0(8,FN1,0x5a827999,DX,tmp2+TMP2(SP),AX,SI,CX)
	BODY0(12,FN1,0x5a827999,CX,DX,tmp2+TMP2(SP),AX,SI)
	MOVL SI,tmp1+TMP1(SP)
	BODY0(16,FN1,0x5a827999,SI,CX,DX,tmp2+TMP2(SP),AX)
	MOVL tmp2+TMP2(SP),SI

	ADDL $20,BX
	ADDL $20,BP
	CMPL BP,w15+W15(SP)
	JCS loop1

	BODY0(0,FN1,0x5a827999,AX,tmp1+TMP1(SP),CX,DX,SI)
	ADDL $4,BX
	MOVL BX,data+DATA(FP)
	MOVL tmp1+TMP1(SP),BX

	BODY(4,FN1,0x5a827999,SI,AX,BX,CX,DX)
	BODY(8,FN1,0x5a827999,DX,SI,AX,BX,CX)
	BODY(12,FN1,0x5a827999,CX,DX,SI,AX,BX)
	BODY(16,FN1,0x5a827999,BX,CX,DX,SI,AX)

	ADDL $20,BP

loop2:
	BODY(0,FN24,0x6ed9eba1,AX,BX,CX,DX,SI)
	BODY(4,FN24,0x6ed9eba1,SI,AX,BX,CX,DX)
	BODY(8,FN24,0x6ed9eba1,DX,SI,AX,BX,CX)
	BODY(12,FN24,0x6ed9eba1,CX,DX,SI,AX,BX)
	BODY(16,FN24,0x6ed9eba1,BX,CX,DX,SI,AX)

	ADDL $20,BP
	CMPL BP,w40+W40(SP)
	JCS loop2

loop3:
	BODY(0,FN3,0x8f1bbcdc,AX,BX,CX,DX,SI)
	BODY(4,FN3,0x8f1bbcdc,SI,AX,BX,CX,DX)
	BODY(8,FN3,0x8f1bbcdc,DX,SI,AX,BX,CX)
	BODY(12,FN3,0x8f1bbcdc,CX,DX,SI,AX,BX)
	BODY(16,FN3,0x8f1bbcdc,BX,CX,DX,SI,AX)

	ADDL $20,BP
	CMPL BP,w60+W60(SP)
	JCS loop3

loop4:
	BODY(0,FN24,0xca62c1d6,AX,BX,CX,DX,SI)
	BODY(4,FN24,0xca62c1d6,SI,AX,BX,CX,DX)
	BODY(8,FN24,0xca62c1d6,DX,SI,AX,BX,CX)
	BODY(12,FN24,0xca62c1d6,CX,DX,SI,AX,BX)
	BODY(16,FN24,0xca62c1d6,BX,CX,DX,SI,AX)

	ADDL $20,BP
	CMPL BP,w80+W80(SP)
	JCS loop4

	MOVL state+STATE(FP),DI
	ADDL AX,0(DI)
	ADDL BX,4(DI)
	ADDL CX,8(DI)
	ADDL DX,12(DI)
	ADDL SI,16(DI)

	MOVL edata+EDATA(SP),DI
	CMPL data+DATA(FP),DI
	JCS mainloop

	RET
	END
