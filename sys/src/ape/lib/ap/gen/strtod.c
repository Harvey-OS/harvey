#include <math.h>
#include <errno.h>

/*
 * bug: should detect overflow, set errno = ERANGE, and return +/- HUGE_VAL
 */
double
strtod(const char *cp, char **endptr)
{
	double num, dem;
	extern double pow10(int);
	int neg, eneg, dig, predig, exp, c;
	const char *p;

	p = cp;
	num = 0;
	neg = 0;
	dig = 0;
	predig = 0;
	exp = 0;
	eneg = 0;

	c = *p++;
	while(c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\v' || c == '\r')
		c = *p++;
	if(c == '-' || c == '+'){
		if(c == '-')
			neg = 1;
		c = *p++;
	}
	while(c >= '0' && c <= '9'){
		num = num*10 + c-'0';
		predig++;
		c = *p++;
	}
	if(c == '.')
		c = *p++;
	while(c >= '0' && c <= '9'){
		num = num*10 + c-'0';
		dig++;
		c = *p++;
	}
	if(dig+predig == 0){
		if(endptr)
			*endptr = (char *)cp;
		return 0.0;
	}
	if(c == 'e' || c == 'E'){
		c = *p++;
		if(c == '-' || c == '+'){
			if(c == '-'){
				dig = -dig;
				eneg = 1;
			}
			c = *p++;
		}
		while(c >= '0' && c <= '9'){
			exp = exp*10 + c-'0';
			c = *p++;
		}
	}
	exp -= dig;
	if(exp < 0){
		exp = -exp;
		eneg = !eneg;
	}
	dem = pow10(exp);
	if(dem==HUGE_VAL)
		num = eneg? 0.0 : HUGE_VAL;
	else if(dem==0)
		num = eneg? HUGE_VAL : 0.0;
	else if(eneg)
		num /= dem;
	else
		num *= dem;
	if(neg)
		num = -num;
	if(endptr){
		*endptr = (char *)--p;
		/*
		 * Fix cases like 2.3e+
		 */
		while(p > cp){
			c = *--p;
			if(c!='-' && c!='+' && c!='e' && c!='E')
				break;
			(*endptr)--;
		}
	}
	return num;
}
