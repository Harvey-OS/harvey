/*
 *  rfc1321 requires that I include this.  The code is new.  The constants
 *  all come from the rfc (hence the copyright).  We trade a table for the
 *  macros in rfc.  The total size is a lot less. -- presotto
 *
 *	Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 *	rights reserved.
 *
 *	License to copy and use this software is granted provided that it
 *	is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 *	Algorithm" in all material mentioning or referencing this software
 *	or this function.
 *
 *	License is also granted to make and use derivative works provided
 *	that such works are identified as "derived from the RSA Data
 *	Security, Inc. MD5 Message-Digest Algorithm" in all material
 *	mentioning or referencing the derived work.
 *
 *	RSA Data Security, Inc. makes no representations concerning either
 *	the merchantability of this software or the suitability of this
 *	software forany particular purpose. It is provided "as is"
 *	without express or implied warranty of any kind.
 *	These notices must be retained in any copies of any part of this
 *	documentation and/or software.
 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22

#define S21 5
#define S22 9
#define S23 14
#define S24 20

#define S31 4
#define S32 11
#define S33 16
#define S34 23

#define S41 6
#define S42 10
#define S43 15
#define S44 21

/*
 * SI is data
 *	a += FN(B,C,D);
 *	a += x[sh] + t[sh];
 *	a = (a << S11) | (a >> (32 - S11));
 *	a += b;
 */

#define BODY1(off,V,FN,SH,A,B,C,D)\
	FN(B,C,D)\
	LEAL V(A)(DI*1),A;\
	ADDL (off)(BP),A;\
	ROLL $SH,A;\
	ADDL B,A;\

#define BODY(off,V,FN,SH,A,B,C,D)\
	FN(B,C,D)\
	LEAL V(A)(DI*1),A;\
	ADDL (off)(BP),A;\
	ROLL $SH,A;\
	ADDL B,A;\

/*
 * fn1 = ((c ^ d) & b) ^ d
 */
#define FN1(B,C,D)\
	MOVL C,DI;\
	XORL D,DI;\
	ANDL B,DI;\
	XORL D,DI;\

/*
 * fn2 = ((b ^ c) & d) ^ c;
 */
#define FN2(B,C,D)\
	MOVL B,DI;\
	XORL C,DI;\
	ANDL D,DI;\
	XORL C,DI;\

/*
 * fn3 = b ^ c ^ d;
 */
#define FN3(B,C,D)\
	MOVL B,DI;\
	XORL C,DI;\
	XORL D,DI;\

/*
 * fn4 = c ^ (b | ~d);
 */
#define FN4(B,C,D)\
	MOVL D,DI;\
	XORL $-1,DI;\
	ORL B,DI;\
	XORL C,DI;\

#define	DATA	0
#define	LEN	4
#define	STATE	8

#define EDATA	(-4)

	TEXT	_md5block+0(SB),$4

	MOVL	data+DATA(FP),AX
	ADDL	len+LEN(FP),AX
	MOVL	AX,edata+EDATA(SP)

	MOVL data+DATA(FP),BP

mainloop:
	MOVL state+STATE(FP),SI
	MOVL (SI),AX
	MOVL 4(SI),BX
	MOVL 8(SI),CX
	MOVL 12(SI),DX

	BODY1( 0*4,0xd76aa478,FN1,S11,AX,BX,CX,DX)
	BODY1( 1*4,0xe8c7b756,FN1,S12,DX,AX,BX,CX)
	BODY1( 2*4,0x242070db,FN1,S13,CX,DX,AX,BX)
	BODY1( 3*4,0xc1bdceee,FN1,S14,BX,CX,DX,AX)

	BODY1( 4*4,0xf57c0faf,FN1,S11,AX,BX,CX,DX)
	BODY1( 5*4,0x4787c62a,FN1,S12,DX,AX,BX,CX)
	BODY1( 6*4,0xa8304613,FN1,S13,CX,DX,AX,BX)
	BODY1( 7*4,0xfd469501,FN1,S14,BX,CX,DX,AX)

	BODY1( 8*4,0x698098d8,FN1,S11,AX,BX,CX,DX)
	BODY1( 9*4,0x8b44f7af,FN1,S12,DX,AX,BX,CX)
	BODY1(10*4,0xffff5bb1,FN1,S13,CX,DX,AX,BX)
	BODY1(11*4,0x895cd7be,FN1,S14,BX,CX,DX,AX)

	BODY1(12*4,0x6b901122,FN1,S11,AX,BX,CX,DX)
	BODY1(13*4,0xfd987193,FN1,S12,DX,AX,BX,CX)
	BODY1(14*4,0xa679438e,FN1,S13,CX,DX,AX,BX)
	BODY1(15*4,0x49b40821,FN1,S14,BX,CX,DX,AX)


	BODY( 1*4,0xf61e2562,FN2,S21,AX,BX,CX,DX)
	BODY( 6*4,0xc040b340,FN2,S22,DX,AX,BX,CX)
	BODY(11*4,0x265e5a51,FN2,S23,CX,DX,AX,BX)
	BODY( 0*4,0xe9b6c7aa,FN2,S24,BX,CX,DX,AX)

	BODY( 5*4,0xd62f105d,FN2,S21,AX,BX,CX,DX)
	BODY(10*4,0x02441453,FN2,S22,DX,AX,BX,CX)
	BODY(15*4,0xd8a1e681,FN2,S23,CX,DX,AX,BX)
	BODY( 4*4,0xe7d3fbc8,FN2,S24,BX,CX,DX,AX)

	BODY( 9*4,0x21e1cde6,FN2,S21,AX,BX,CX,DX)
	BODY(14*4,0xc33707d6,FN2,S22,DX,AX,BX,CX)
	BODY( 3*4,0xf4d50d87,FN2,S23,CX,DX,AX,BX)
	BODY( 8*4,0x455a14ed,FN2,S24,BX,CX,DX,AX)

	BODY(13*4,0xa9e3e905,FN2,S21,AX,BX,CX,DX)
	BODY( 2*4,0xfcefa3f8,FN2,S22,DX,AX,BX,CX)
	BODY( 7*4,0x676f02d9,FN2,S23,CX,DX,AX,BX)
	BODY(12*4,0x8d2a4c8a,FN2,S24,BX,CX,DX,AX)


	BODY( 5*4,0xfffa3942,FN3,S31,AX,BX,CX,DX)
	BODY( 8*4,0x8771f681,FN3,S32,DX,AX,BX,CX)
	BODY(11*4,0x6d9d6122,FN3,S33,CX,DX,AX,BX)
	BODY(14*4,0xfde5380c,FN3,S34,BX,CX,DX,AX)

	BODY( 1*4,0xa4beea44,FN3,S31,AX,BX,CX,DX)
	BODY( 4*4,0x4bdecfa9,FN3,S32,DX,AX,BX,CX)
	BODY( 7*4,0xf6bb4b60,FN3,S33,CX,DX,AX,BX)
	BODY(10*4,0xbebfbc70,FN3,S34,BX,CX,DX,AX)

	BODY(13*4,0x289b7ec6,FN3,S31,AX,BX,CX,DX)
	BODY( 0*4,0xeaa127fa,FN3,S32,DX,AX,BX,CX)
	BODY( 3*4,0xd4ef3085,FN3,S33,CX,DX,AX,BX)
	BODY( 6*4,0x04881d05,FN3,S34,BX,CX,DX,AX)

	BODY( 9*4,0xd9d4d039,FN3,S31,AX,BX,CX,DX)
	BODY(12*4,0xe6db99e5,FN3,S32,DX,AX,BX,CX)
	BODY(15*4,0x1fa27cf8,FN3,S33,CX,DX,AX,BX)
	BODY( 2*4,0xc4ac5665,FN3,S34,BX,CX,DX,AX)


	BODY( 0*4,0xf4292244,FN4,S41,AX,BX,CX,DX)
	BODY( 7*4,0x432aff97,FN4,S42,DX,AX,BX,CX)
	BODY(14*4,0xab9423a7,FN4,S43,CX,DX,AX,BX)
	BODY( 5*4,0xfc93a039,FN4,S44,BX,CX,DX,AX)

	BODY(12*4,0x655b59c3,FN4,S41,AX,BX,CX,DX)
	BODY( 3*4,0x8f0ccc92,FN4,S42,DX,AX,BX,CX)
	BODY(10*4,0xffeff47d,FN4,S43,CX,DX,AX,BX)
	BODY( 1*4,0x85845dd1,FN4,S44,BX,CX,DX,AX)

	BODY( 8*4,0x6fa87e4f,FN4,S41,AX,BX,CX,DX)
	BODY(15*4,0xfe2ce6e0,FN4,S42,DX,AX,BX,CX)
	BODY( 6*4,0xa3014314,FN4,S43,CX,DX,AX,BX)
	BODY(13*4,0x4e0811a1,FN4,S44,BX,CX,DX,AX)

	BODY( 4*4,0xf7537e82,FN4,S41,AX,BX,CX,DX)
	BODY(11*4,0xbd3af235,FN4,S42,DX,AX,BX,CX)
	BODY( 2*4,0x2ad7d2bb,FN4,S43,CX,DX,AX,BX)
	BODY( 9*4,0xeb86d391,FN4,S44,BX,CX,DX,AX)

	ADDL $(16*4),BP
	MOVL state+STATE(FP),DI
	ADDL AX,0(DI)
	ADDL BX,4(DI)
	ADDL CX,8(DI)
	ADDL DX,12(DI)

	MOVL edata+EDATA(SP),DI
	CMPL BP,DI
	JCS mainloop

	RET

	END
