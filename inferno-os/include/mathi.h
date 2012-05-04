/*
 * floating point control and status register masks
 */
enum
{
	INVAL		= 0x0001,
	ZDIV		= 0x0002,
	OVFL		= 0x0004,
	UNFL		= 0x0008,
	INEX		= 0x0010,
	RND_NR		= 0x0000,
	RND_NINF	= 0x0100,
	RND_PINF	= 0x0200,
	RND_Z		= 0x0300,
	RND_MASK	= 0x0300
};

extern	double	ipow10(int);
extern	void	FPinit(void);
extern	double	dot(int, double*, double*);
extern	ulong	FPcontrol(ulong, ulong);
extern	ulong	FPstatus(ulong, ulong);
extern	void	gemm(int, int, int, int, int, double,
			double*, int, double*, int, double, double*, int);
extern	ulong	getFPstatus(void);
extern	ulong	getFPcontrol(void);
extern	char*	g_fmt(char *, double, int);
extern	int	iamax(int, double*);
extern	double	fdim(double, double);
extern	double	fmax(double, double);
extern	double	fmin(double, double);
extern	double	norm2(int, double*);
extern	double	norm1(int, double*);
extern	double	strtod(const char *, char **);

/* fdlibm */
extern double __ieee754_acos(double);
extern double __ieee754_acosh(double);
extern double __ieee754_asin(double);
extern double asinh(double);
extern double atan(double);
extern double __ieee754_atan2(double, double);
extern double __ieee754_atanh(double);
extern double cbrt(double);
extern double ceil(double);
extern double copysign(double, double);
extern double cos(double);
extern double __ieee754_cosh(double);
extern double erf(double);
extern double erfc(double);
extern double __ieee754_exp(double);
extern double expm1(double);
extern double fabs(double);
extern int finite(double);
extern double floor(double);
extern double __ieee754_fmod(double, double);
extern double __ieee754_hypot(double, double);
extern int ilogb(double);
extern int isnan(double);
extern double __ieee754_j0(double);
extern double __ieee754_j1(double);
extern double __ieee754_jn(int, double);
extern double __ieee754_lgamma_r(double,int*);
extern double __ieee754_log(double);
extern double __ieee754_log10(double);
extern double log1p(double);
extern double logb(double);
extern double modf(double, double *);
extern double nextafter(double, double);
extern double __ieee754_pow(double, double);
extern double __ieee754_remainder(double, double);
extern double rint(double);
extern double scalbn(double, int);
extern double sin(double);
extern double __ieee754_sinh(double);
extern double __ieee754_sqrt(double);
extern double tan(double);
extern double tanh(double);
extern double __ieee754_y0(double);
extern double __ieee754_y1(double);
extern double __ieee754_yn(int, double);
