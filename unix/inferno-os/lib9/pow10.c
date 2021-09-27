#ifdef	LINUX_386
#define	_MATH_H
#endif

#include	"lib9.h"

/*
 * this table might overflow 127-bit exponent representations.
 * in that case, truncate it after 1.0e38.
 * it is important to get all one can from this
 * routine since it is used in atof to scale numbers.
 * the presumption is that C converts fp numbers better
 * than multipication of lower powers of 10.
 */

static
double	tab[] =
{
	1.0e0, 1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e6, 1.0e7, 1.0e8, 1.0e9,
	1.0e10,1.0e11,1.0e12,1.0e13,1.0e14,1.0e15,1.0e16,1.0e17,1.0e18,1.0e19,
	1.0e20,1.0e21,1.0e22,1.0e23,1.0e24,1.0e25,1.0e26,1.0e27,1.0e28,1.0e29,
	1.0e30,1.0e31,1.0e32,1.0e33,1.0e34,1.0e35,1.0e36,1.0e37,1.0e38,1.0e39,
	1.0e40,1.0e41,1.0e42,1.0e43,1.0e44,1.0e45,1.0e46,1.0e47,1.0e48,1.0e49,
	1.0e50,1.0e51,1.0e52,1.0e53,1.0e54,1.0e55,1.0e56,1.0e57,1.0e58,1.0e59,
	1.0e60,1.0e61,1.0e62,1.0e63,1.0e64,1.0e65,1.0e66,1.0e67,1.0e68,1.0e69,
};

double
pow10(int n)
{
	int m;

	if(n < 0) {
		n = -n;
		if(n < sizeof(tab)/sizeof(tab[0]))
			return 1/tab[n];
		m = n/2;
		return pow10(-m) * pow10(m-n);
	}
	if(n < sizeof(tab)/sizeof(tab[0]))
		return tab[n];
	m = n/2;
	return pow10(m) * pow10(n-m);
}
