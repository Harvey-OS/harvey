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
	struct {	/* little endian */
		long lo;
		long hi;
	};
};

#ifdef _RESEARCH_SOURCE
/* define stuff needed for floating conversion */
#define IEEE_8087	1
#define Sudden_Underflow 1
#endif
#ifdef _PLAN9_SOURCE
#define	FPINEX	(1<<30)
#define	FPOVFL	(1<<19)
#define	FPUNFL	((1<<29)|(1<<28))
#define	FPZDIV	(1<<18)
#define	FPINVAL	(1<<17)

#define	FPRNR	(2<<26)
#define	FPRZ		(0<<26)
#define	FPRPINF	(3<<26)
#define	FPRNINF	(1<<26)
#define	FPRMASK	(3<<26)

#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<24)
#define	FPAUNFL	(1<<23)
#define	FPAOVFL	(1<<22)
#define	FPAZDIV	(1<<21)
#define	FPAINVAL	(1<<20)
#endif
#endif /* __FLOAT */
