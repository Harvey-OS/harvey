#include	<alef.h>

#define	whitespace(c)	((c) == ' ' || (c) == '\t')

tuple(int, uint, byte*)
Tstrtoui(byte* str, int base)
{
	int c, val, top;

	while(*str != 0 && whitespace(*str))
		str++;
	if(str == nil || *str == 0)
		return(0, 0, str);
	val = 0;
	top = 9;
	if(base < 10)
		top = base-1;
	while(*str && !whitespace(*str)) {
		c = *str;
		if (c >= '0' && c <= top+'0')
			val = val*base+(c-'0');
		else
		if(base > 10)
			if(c <= base-10+'a')
				val = val*base+(c-'a'+10);
			else
			if(c <= base-10+'A')
				val = val*base+(c-'A'+10);
			else
				return (-1, val, str+1);	/* error */
		else
			return (-1, val, str+1);		/* error */
		str++;
	}
	return(1, val, str);
}

void
main(void)
{
	int ret;
	uint val;
	byte *p, *newp;

	p = "123 456	7a8";
	while(*p) {
		(ret, val, newp) = Tstrtoui(p, 16);
		if(ret == 0)
			break;
		if(ret < 0) {
			*newp = 0;
			print("bad number %s\n", p);
		} else
			print("%d\n", val);
		p = newp;
	}
	exits(nil);
}
