#include <alef.h>

float
strtof(byte *s, byte **sp)
{
	float num, dem;
	int neg, eneg, dig, exp, c;

	num = 0;
	neg = 0;
	dig = 0;
	exp = 0;
	eneg = 0;

	c = *s++;
	while(c == ' ' || c == '\t')
		c = *s++;
	if(c == '-' || c == '+'){
		if(c == '-')
			neg = 1;
		c = *s++;
	}
	while(c >= '0' && c <= '9'){
		num = num*10 + c-'0';
		c = *s++;
	}
	if(c == '.')
		c = *s++;
	while(c >= '0' && c <= '9'){
		num = num*10 + c-'0';
		dig++;
		c = *s++;
	}
	if(c == 'e' || c == 'E'){
		if(s[0] == '-' || s[0] == '+')
			c = s[1] >= '0' && s[1] <= '9';
		else
			c = s[0] >= '0' && s[0] <= '9';
		if(c){
			c = *s++;
			if(c == '-' || c == '+'){
				if(c == '-'){
					dig = -dig;
					eneg = 1;
				}
				c = *s++;
			}
			while(c >= '0' && c <= '9'){
				exp = exp*10 + c-'0';
				c = *s++;
			}
		}
	}
	exp -= dig;
	if(exp < 0){
		exp = -exp;
		eneg = !eneg;
	}
	dem = pow10(exp);
	if(eneg)
		num /= dem;
	else
		num *= dem;
	if(sp)
		*sp = s-1;
	if(neg)
		return -num;
	return num;
}
