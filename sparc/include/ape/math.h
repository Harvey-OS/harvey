#ifndef __MATH
#define __MATH
#pragma lib "/$M/lib/ape/libap.a"

/* a HUGE_VAL appropriate for IEEE double-precision */
/* the correct value, 1.797693134862316e+308, causes a ken overflow */
#define HUGE_VAL 1.79769313486231e+308

#ifdef __cplusplus
extern "C" {
#endif

extern double acos(double);
extern double asin(double);
extern double atan(double);
extern double atan2(double, double);
extern double cos(double);
extern double sin(double);
extern double tan(double);
extern double cosh(double);
extern double sinh(double);
extern double tanh(double);
extern double exp(double);
extern double frexp(double, int *);
extern double ldexp(double, int);
extern double log(double);
extern double log10(double);
extern double modf(double, double *);
extern double pow(double, double);
extern double sqrt(double);
extern double ceil(double);
extern double fabs(double);
extern double floor(double);
extern double fmod(double, double);
extern double NaN(void);
extern int isNaN(double);
extern double Inf(int);
extern int isInf(double, int);

#ifdef __cplusplus
}
#endif


#ifdef _RESEARCH_SOURCE
/* does >> treat left operand as unsigned ? */
#define Unsigned_Shifts 1

extern double hypot(double, double);
#endif
#endif /* __MATH */
