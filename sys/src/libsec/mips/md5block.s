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

	/* round 1 */
	DATA	md5tab<>+( 0*4)(SB)/4,$0xd76aa478	
	DATA	md5tab<>+( 1*4)(SB)/4,$0xe8c7b756	
	DATA	md5tab<>+( 2*4)(SB)/4,$0x242070db	
	DATA	md5tab<>+( 3*4)(SB)/4,$0xc1bdceee	
	DATA	md5tab<>+( 4*4)(SB)/4,$0xf57c0faf	
	DATA	md5tab<>+( 5*4)(SB)/4,$0x4787c62a	
	DATA	md5tab<>+( 6*4)(SB)/4,$0xa8304613	
	DATA	md5tab<>+( 7*4)(SB)/4,$0xfd469501	
	DATA	md5tab<>+( 8*4)(SB)/4,$0x698098d8	
	DATA	md5tab<>+( 9*4)(SB)/4,$0x8b44f7af	
	DATA	md5tab<>+(10*4)(SB)/4,$0xffff5bb1	
	DATA	md5tab<>+(11*4)(SB)/4,$0x895cd7be	
	DATA	md5tab<>+(12*4)(SB)/4,$0x6b901122	
	DATA	md5tab<>+(13*4)(SB)/4,$0xfd987193	
	DATA	md5tab<>+(14*4)(SB)/4,$0xa679438e	
	DATA	md5tab<>+(15*4)(SB)/4,$0x49b40821

	/* round 2 */
	DATA	md5tab<>+(16*4)(SB)/4,$0xf61e2562	
	DATA	md5tab<>+(17*4)(SB)/4,$0xc040b340	
	DATA	md5tab<>+(18*4)(SB)/4,$0x265e5a51	
	DATA	md5tab<>+(19*4)(SB)/4,$0xe9b6c7aa	
	DATA	md5tab<>+(20*4)(SB)/4,$0xd62f105d	
	DATA	md5tab<>+(21*4)(SB)/4,$0x02441453	
	DATA	md5tab<>+(22*4)(SB)/4,$0xd8a1e681	
	DATA	md5tab<>+(23*4)(SB)/4,$0xe7d3fbc8	
	DATA	md5tab<>+(24*4)(SB)/4,$0x21e1cde6	
	DATA	md5tab<>+(25*4)(SB)/4,$0xc33707d6	
	DATA	md5tab<>+(26*4)(SB)/4,$0xf4d50d87	
	DATA	md5tab<>+(27*4)(SB)/4,$0x455a14ed	
	DATA	md5tab<>+(28*4)(SB)/4,$0xa9e3e905	
	DATA	md5tab<>+(29*4)(SB)/4,$0xfcefa3f8	
	DATA	md5tab<>+(30*4)(SB)/4,$0x676f02d9	
	DATA	md5tab<>+(31*4)(SB)/4,$0x8d2a4c8a

	/* round 3 */
	DATA	md5tab<>+(32*4)(SB)/4,$0xfffa3942	
	DATA	md5tab<>+(33*4)(SB)/4,$0x8771f681	
	DATA	md5tab<>+(34*4)(SB)/4,$0x6d9d6122	
	DATA	md5tab<>+(35*4)(SB)/4,$0xfde5380c	
	DATA	md5tab<>+(36*4)(SB)/4,$0xa4beea44	
	DATA	md5tab<>+(37*4)(SB)/4,$0x4bdecfa9	
	DATA	md5tab<>+(38*4)(SB)/4,$0xf6bb4b60	
	DATA	md5tab<>+(39*4)(SB)/4,$0xbebfbc70	
	DATA	md5tab<>+(40*4)(SB)/4,$0x289b7ec6	
	DATA	md5tab<>+(41*4)(SB)/4,$0xeaa127fa	
	DATA	md5tab<>+(42*4)(SB)/4,$0xd4ef3085	
	DATA	md5tab<>+(43*4)(SB)/4,$0x04881d05	
	DATA	md5tab<>+(44*4)(SB)/4,$0xd9d4d039	
	DATA	md5tab<>+(45*4)(SB)/4,$0xe6db99e5	
	DATA	md5tab<>+(46*4)(SB)/4,$0x1fa27cf8	
	DATA	md5tab<>+(47*4)(SB)/4,$0xc4ac5665	

	/* round 4 */
	DATA	md5tab<>+(48*4)(SB)/4,$0xf4292244	
	DATA	md5tab<>+(49*4)(SB)/4,$0x432aff97	
	DATA	md5tab<>+(50*4)(SB)/4,$0xab9423a7	
	DATA	md5tab<>+(51*4)(SB)/4,$0xfc93a039	
	DATA	md5tab<>+(52*4)(SB)/4,$0x655b59c3	
	DATA	md5tab<>+(53*4)(SB)/4,$0x8f0ccc92	
	DATA	md5tab<>+(54*4)(SB)/4,$0xffeff47d	
	DATA	md5tab<>+(55*4)(SB)/4,$0x85845dd1	
	DATA	md5tab<>+(56*4)(SB)/4,$0x6fa87e4f	
	DATA	md5tab<>+(57*4)(SB)/4,$0xfe2ce6e0	
	DATA	md5tab<>+(58*4)(SB)/4,$0xa3014314	
	DATA	md5tab<>+(59*4)(SB)/4,$0x4e0811a1	
	DATA	md5tab<>+(60*4)(SB)/4,$0xf7537e82	
	DATA	md5tab<>+(61*4)(SB)/4,$0xbd3af235	
	DATA	md5tab<>+(62*4)(SB)/4,$0x2ad7d2bb	
	DATA	md5tab<>+(63*4)(SB)/4,$0xeb86d391

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

#define	AREG		R5
#define BREG		R6
#define CREG		R7
#define DREG		R8
#define DATAREG		R1
#define TABREG		R10
#define STREG		R11
#define XREG		R12
#define ELOOPREG	R13
#define EDREG		R14
#define IREG		R15

#define TMP1		R9
#define TMP2		R2
#define TMP3		R3
#define TMP4		R4

/*
 * decode little endian data into x[off], then the body
 * bodies have this form:
 *	a += FN(B,C,D);
 *	a += x[off] + t[off];
 *	a = (a << S11) | (a >> (32 - S11));
 *	a += b;
 */
#define BODY1(off,FN,SH,A,B,C,D)\
	MOVBU off(DATAREG),TMP2;\
	MOVBU (off+1)(DATAREG),TMP3;\
	MOVBU (off+2)(DATAREG),TMP1;\
	MOVBU (off+3)(DATAREG),TMP4;\
	SLL $8,TMP3;\
	OR TMP3,TMP2;\
	SLL $16,TMP1;\
	OR TMP1,TMP2;\
	SLL $24,TMP4;\
	OR TMP4,TMP2;\
	MOVW off(TABREG),TMP3;\
	FN(B,C,D)\
	ADDU TMP1,A;\
	MOVW TMP2,off(XREG);\
	ADDU TMP2,A;\
	ADDU TMP3,A;\
	SLL $SH,A,TMP1;\
	SRL $(32-SH),A;\
	OR TMP1,A;\
	ADDU B,A;\

#define BODY(off,inc,FN,SH,A,B,C,D)\
	MOVW off(TABREG),TMP3;\
	ADDU XREG,IREG,TMP4;\
	MOVW (TMP4),TMP2;\
	ADDU $(inc*4),IREG;\
	AND $63,IREG;\
	FN(B,C,D)\
	ADDU TMP1,A;\
	ADDU TMP2,A;\
	ADDU TMP3,A;\
	SLL $SH,A,TMP1;\
	SRL $(32-SH),A;\
	OR  TMP1,A;\
	ADDU B,A;\

/*
 * fn1 = ((c ^ d) & b) ^ d
 */
#define FN1(B,C,D)\
	XOR C,D,TMP1;\
	AND B,TMP1;\
	XOR D,TMP1;\

/*
 * fn2 = ((b ^ c) & d) ^ c;
 */
#define FN2(B,C,D)\
	XOR B,C,TMP1;\
	AND D,TMP1;\
	XOR C,TMP1;\

/*
 * fn3 = b ^ c ^ d;
 */
#define FN3(B,C,D)\
	XOR B,C,TMP1;\
	XOR D,TMP1;\

/*
 * fn4 = c ^ (b | ~d);
 */
#define FN4(B,C,D)\
	XOR $-1,D,TMP1;\
	OR B,TMP1;\
	XOR C,TMP1;\

#define	DATA	0
#define	LEN	4
#define	STATE	8

#define XOFF	(-4-16*4)

	TEXT	_md5block+0(SB),$68

	MOVW	len+LEN(FP),TMP1
	ADDU	DATAREG,TMP1,EDREG
	MOVW	state+STATE(FP),STREG

	MOVW 0(STREG),AREG
	MOVW 4(STREG),BREG
	MOVW 8(STREG),CREG
	MOVW 12(STREG),DREG

mainloop:

	MOVW $md5tab<>+0(SB),TABREG
	ADDU $(16*4),DATAREG,ELOOPREG
	MOVW $x+XOFF(SP),XREG

loop1:
	BODY1(0,FN1,S11,AREG,BREG,CREG,DREG)
	BODY1(4,FN1,S12,DREG,AREG,BREG,CREG)
	BODY1(8,FN1,S13,CREG,DREG,AREG,BREG)
	BODY1(12,FN1,S14,BREG,CREG,DREG,AREG)

	ADDU $16,DATAREG
	ADDU $16,TABREG
	ADDU $16,XREG

	BNE DATAREG,ELOOPREG,loop1


	MOVW $x+XOFF(SP),XREG
	MOVW $(1*4),IREG
	MOVW $(1*4),ELOOPREG
loop2:
	BODY(0,5,FN2,S21,AREG,BREG,CREG,DREG)
	BODY(4,5,FN2,S22,DREG,AREG,BREG,CREG)
	BODY(8,5,FN2,S23,CREG,DREG,AREG,BREG)
	BODY(12,5,FN2,S24,BREG,CREG,DREG,AREG)

	ADDU $16,TABREG

	BNE IREG,ELOOPREG,loop2


	MOVW $(5*4),IREG
	MOVW $(5*4),ELOOPREG
loop3:
	BODY(0,3,FN3,S31,AREG,BREG,CREG,DREG)
	BODY(4,3,FN3,S32,DREG,AREG,BREG,CREG)
	BODY(8,3,FN3,S33,CREG,DREG,AREG,BREG)
	BODY(12,3,FN3,S34,BREG,CREG,DREG,AREG)

	ADDU $16,TABREG

	BNE IREG,ELOOPREG,loop3


	MOVW $0,IREG
loop4:
	BODY(0,7,FN4,S41,AREG,BREG,CREG,DREG)
	BODY(4,7,FN4,S42,DREG,AREG,BREG,CREG)
	BODY(8,7,FN4,S43,CREG,DREG,AREG,BREG)
	BODY(12,7,FN4,S44,BREG,CREG,DREG,AREG)

	ADDU $16,TABREG

	BNE IREG,R0,loop4

	MOVW 0(STREG),TMP1
	MOVW 4(STREG),TMP2
	MOVW 8(STREG),TMP3
	MOVW 12(STREG),TMP4
	ADDU TMP1,AREG
	ADDU TMP2,BREG
	ADDU TMP3,CREG
	ADDU TMP4,DREG
	MOVW AREG,0(STREG)
	MOVW BREG,4(STREG)
	MOVW CREG,8(STREG)
	MOVW DREG,12(STREG)

	BNE DATAREG,EDREG,mainloop

	RET

	GLOBL	md5tab<>+0(SB),$256

	END
