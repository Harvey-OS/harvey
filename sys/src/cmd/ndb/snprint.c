#include <u.h>
#include <libc.h>

/*
 *  this could be done by sprint(), but this is totally reentrant
 *  allowing shared memory applications to call dial().
 */
int
snprint(char *p, int len, char *fmt, ...)
{
	char **argv;
	char *start, *e, *s;
	char numbuf[32];
	int x;
	char sign;

	start = p;
	argv = (&fmt+1);
	for(e = p + len - 1; *fmt && p < e;){
		if(*fmt != '%'){
			*p++ = *fmt++;
			continue;
		}
		switch(*(fmt+1)){
		case 's':
			fmt += 2;
			s = *argv++;
			while(*s && p < e)
				*p++ = *s++;
			break;
		case 'd':
			fmt += 2;
			sign = x = (int)(*argv++);
			if(x < 0)
				x = -x;
			for(s = numbuf; x > 0; x /= 10)
				*s++ = (x%10) + '0';
			if(sign < 0)
				*p++ = '-';
			else if(s == numbuf)
				*p++ = '0';
			while(--s >= numbuf && p < e)
				*p++ = *s;
			break;
		default:
			*p++ = *fmt++;
			while(*fmt && p < e)
				*p++ = *fmt++;
			break;
		}
	}
	*p = 0;
	return p - start;
}
