#ifndef __FLOAT
#define __FLOAT
/* IEEE, default rounding */

#define FLT_ROUNDS	1
#define FLT_RADIX	2

#define FLT_DIG		6
#define FLT_EPSILON	1.19209290e-07
#define FLT_MANT_DIG	24
#define FLT_MAX		3.40282347e+38
#define FLT_MAX_10_EXP	38
#define FLT_MAX_EXP	128
#define FLT_MIN		1.17549435e-38
#define FLT_MIN_10_EXP	-37
#define FLT_MIN_EXP	-125

#define DBL_DIG		15
#define DBL_EPSILON	2.2204460492503131e-16
#define DBL_MANT_DIG	53
#define DBL_MAX		1.797693134862315708145e+308
#define DBL_MAX_10_EXP	308
#define DBL_MAX_EXP	1024
#define DBL_MIN		2.225073858507201383090233e-308
#define DBL_MIN_10_EXP	-307
#define DBL_MIN_EXP	-1021
#define LDBL_MANT_DIG	DBL_MANT_DIG
#define LDBL_EPSILON	DBL_EPSILON
#define LDBL_DIG	DBL_DIG
#define LDBL_MIN_EXP	DBL_MIN_EXP
#define LDBL_MIN	DBL_MIN
#define LDBL_MIN_10_EXP	DBL_MIN_10_EXP
#define LDBL_MAX_EXP	DBL_MAX_EXP
#define LDBL_MAX	DBL_MAX
#define LDBL_MAX_10_EXP	DBL_MAX_10_EXP

typedef 	union FPdbleword FPdbleword;
union FPdbleword
{
	double	x;
	struct {	/* big endian */
		long hi;
		long lo;
	};
};

#ifdef _RESEARCH_SOURCE
/* define order of longs in IEEE double: little endian */
#define IEEE_MC68k	1
#define Sudden_Underflow 1
#endif
#ifdef _PLAN9_SOURCE
/* FCR */
#define	FPINEX	(1<<23)
#define	FPOVFL	(1<<26)
#define	FPUNFL	(1<<25)
#define	FPZDIV	(1<<24)
#define	FPRNR	(0<<30)
#define	FPRZ	(1<<30)
#define	FPRPINF	(2<<30)
#define	FPRNINF	(3<<30)
#define	FPRMASK	(3<<30)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<5)
#define	FPAOVFL	(1<<8)
#define	FPAUNFL	(1<<7)
#define	FPAZDIV	(1<<6)
#endif
#endif /* __FLOAT */
