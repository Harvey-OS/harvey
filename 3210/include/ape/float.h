#ifndef __FLOAT
#define __FLOAT
/* dsp, trunc to 0 rounding */

#define	FLT_ROUNDS	1	/* correct???? */
#define	FLT_RADIX	2

#define	FLT_DIG		6
#define	FLT_EPSILON	1.19209290e-07
#define	FLT_MANT_DIG	24
#define	FLT_MAX		3.40282347e+38
#define	FLT_MAX_10_EXP	38
#define	FLT_MAX_EXP	128
#define	FLT_MIN		5.87747175e-39
#define	FLT_MIN_10_EXP	-38		/* min n st. exp(10, n) is ok */
#define	FLT_MIN_EXP	-127

#define	DBL_DIG		FLT_DIG
#define	DBL_EPSILON	FLT_EPSILON
#define	DBL_MANT_DIG	DBL_MANT_DIG
#define	DBL_MAX		DBL_MAX
#define	DBL_MAX_10_EXP	DBL_MAX_10_EXP
#define	DBL_MAX_EXP	DBL_MAX_EXP
#define	DBL_MIN		DBL_MIN
#define	DBL_MIN_10_EXP	DBL_MIN_10_EXP
#define	DBL_MIN_EXP	DBL_MIN_EXP
#define LDBL_MANT_DIG	DBL_MANT_DIG
#define LDBL_EPSILON	DBL_EPSILON
#define LDBL_DIG	DBL_DIG
#define LDBL_MIN_EXP	DBL_MIN_EXP
#define LDBL_MIN	DBL_MIN
#define LDBL_MIN_10_EXP	DBL_MIN_10_EXP
#define LDBL_MAX_EXP	DBL_MAX_EXP
#define LDBL_MAX	DBL_MAX
#define LDBL_MAX_10_EXP	DBL_MAX_10_EXP

#ifdef _RESEARCH_SOURCE
/* define stuff needed for floating conversion */
#define IEEE_MC68k	1
#define Sudden_Underflow 1
#endif
#ifdef _PLAN9_SOURCE
/* FCR */
/* could enable overflow and underflow exceptions at same time */
#define	FPINEX	0
#define	FPOVFL	0
#define	FPUNFL	0
#define	FPZDIV	0
#define	FPRNR	(0<<4)
#define	FPRZ	(3<<4)
#define	FPRPINF	0
#define	FPRNINF	(1<<4)
#define	FPRMASK	(3<<4)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	0
#define	FPAOVFL	0
#define	FPAUNFL	0
#define	FPAZDIV	0
#endif
#endif /* __FLOAT */
