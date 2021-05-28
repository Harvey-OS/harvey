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
	MOVLQZX off(BX),DI;\
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
#define	LEN	8
#define	STATE	16

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
#define	Rpdata	R8
#define WARRAY	(-8-(80*4))
#define TMP1	(-16-(80*4))
#define TMP2	(-24-(80*4))
#define W15	(-32-(80*4))
#define W40	(-40-(80*4))
#define W60	(-48-(80*4))
#define W80	(-56-(80*4))
#define EDATA	(-64-(80*4))

TEXT	_sha1block+0(SB),$384

	MOVQ RARG, Rpdata
	MOVLQZX len+LEN(FP),BX
	ADDQ BX, RARG
	MOVQ RARG,edata+EDATA(SP)

	LEAQ aw15+(WARRAY+15*4)(SP),DI
	MOVQ DI,w15+W15(SP)
	LEAQ aw40+(WARRAY+40*4)(SP),DX
	MOVQ DX,w40+W40(SP)
	LEAQ aw60+(WARRAY+60*4)(SP),CX
	MOVQ CX,w60+W60(SP)
	LEAQ aw80+(WARRAY+80*4)(SP),DI
	MOVQ DI,w80+W80(SP)

mainloop:
	LEAQ warray+WARRAY(SP),BP

	MOVQ state+STATE(FP),DI
	MOVL (DI),AX
	MOVL 4(DI),BX
	MOVL BX,tmp1+TMP1(SP)
	MOVL 8(DI),CX
	MOVL 12(DI),DX
	MOVL 16(DI),SI

	MOVQ Rpdata,BX

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

	ADDQ $20,BX
	ADDQ $20,BP
	CMPQ BP,w15+W15(SP)
	JCS loop1

	BODY0(0,FN1,0x5a827999,AX,tmp1+TMP1(SP),CX,DX,SI)
	ADDQ $4,BX
	MOVQ BX,R8
	MOVQ tmp1+TMP1(SP),BX

	BODY(4,FN1,0x5a827999,SI,AX,BX,CX,DX)
	BODY(8,FN1,0x5a827999,DX,SI,AX,BX,CX)
	BODY(12,FN1,0x5a827999,CX,DX,SI,AX,BX)
	BODY(16,FN1,0x5a827999,BX,CX,DX,SI,AX)

	ADDQ $20,BP

loop2:
	BODY(0,FN24,0x6ed9eba1,AX,BX,CX,DX,SI)
	BODY(4,FN24,0x6ed9eba1,SI,AX,BX,CX,DX)
	BODY(8,FN24,0x6ed9eba1,DX,SI,AX,BX,CX)
	BODY(12,FN24,0x6ed9eba1,CX,DX,SI,AX,BX)
	BODY(16,FN24,0x6ed9eba1,BX,CX,DX,SI,AX)

	ADDQ $20,BP
	CMPQ BP,w40+W40(SP)
	JCS loop2

loop3:
	BODY(0,FN3,0x8f1bbcdc,AX,BX,CX,DX,SI)
	BODY(4,FN3,0x8f1bbcdc,SI,AX,BX,CX,DX)
	BODY(8,FN3,0x8f1bbcdc,DX,SI,AX,BX,CX)
	BODY(12,FN3,0x8f1bbcdc,CX,DX,SI,AX,BX)
	BODY(16,FN3,0x8f1bbcdc,BX,CX,DX,SI,AX)

	ADDQ $20,BP
	CMPQ BP,w60+W60(SP)
	JCS loop3

loop4:
	BODY(0,FN24,0xca62c1d6,AX,BX,CX,DX,SI)
	BODY(4,FN24,0xca62c1d6,SI,AX,BX,CX,DX)
	BODY(8,FN24,0xca62c1d6,DX,SI,AX,BX,CX)
	BODY(12,FN24,0xca62c1d6,CX,DX,SI,AX,BX)
	BODY(16,FN24,0xca62c1d6,BX,CX,DX,SI,AX)

	ADDQ $20,BP
	CMPQ BP,w80+W80(SP)
	JCS loop4

	MOVQ state+STATE(FP),DI
	ADDL AX,0(DI)
	ADDL BX,4(DI)
	ADDL CX,8(DI)
	ADDL DX,12(DI)
	ADDL SI,16(DI)

	MOVQ edata+EDATA(SP),DI
	CMPQ Rpdata,DI
	JCS mainloop

	RET
	END
