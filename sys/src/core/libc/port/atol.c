#include <u.h>
#include <libc.h>

long
atol(char *s)
{
	long n;
	int f, c;

	n = 0;
	f = 0;
	while(*s == ' ' || *s == '\t')
		s++;
	if(*s == '-' || *s == '+') {
		if(*s++ == '-')
			f = 1;
		while(*s == ' ' || *s == '\t')
			s++;
	}
	if(s[0]=='0' && s[1]) {
		if(s[1]=='x' || s[1]=='X'){
			s += 2;
			for(;;) {
				c = *s;
				if(c >= '0' && c <= '9')
					n = n*16 + c - '0';
				else
				if(c >= 'a' && c <= 'f')
					n = n*16 + c - 'a' + 10;
				else
				if(c >= 'A' && c <= 'F')
					n = n*16 + c - 'A' + 10;
				else
					break;
				s++;
			}
		} else
			while(*s >= '0' && *s <= '7')
				n = n*8 + *s++ - '0';
	} else
		while(*s >= '0' && *s <= '9')
			n = n*10 + *s++ - '0';
	if(f)
		n = -n;
	return n;
}

int
atoi(char *s)
{

	return atol(s);
}
