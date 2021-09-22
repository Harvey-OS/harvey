/*
 * minimal print for booting
 */
int
print(char *fmt, ...)
{
	int sign;
	long d;
	ulong x;
	char *p, *s, buf[20];
	va_list arg;
	static char hex[] = "0123456789abcdef";

	va_start(arg, fmt);
	for(p = fmt; *p; p++){
		if(*p != '%') {
			cgaputc(*p);
			continue;
		}
		s = "0";
		while (p[1] == 'l' || p[1] == 'u' || p[1] == '#')
			p++;
		switch(*++p){
		case 'p':
		case 'x':
		case 'X':
			x = va_arg(arg, ulong);
			if(x == 0)
				break;
			s = buf+sizeof buf;
			*--s = 0;
			while(x > 0){
				*--s = hex[x&15];
				x /= 16;
			}
			if(s == buf+sizeof buf)
				*--s = '0';
			break;
		case 'd':
			d = va_arg(arg, ulong);
			if(d == 0)
				break;
			if(d < 0){
				d = -d;
				sign = -1;
			}else
				sign = 1;
			s = buf+sizeof buf;
			*--s = 0;
			while(d > 0){
				*--s = (d%10)+'0';
				d /= 10;
			}
			if(sign < 0)
				*--s = '-';
			break;
		case 's':
			s = va_arg(arg, char*);
			break;
		case '\0':
			return 0;
		}
		for(; *s; s++)
			cgaputc(*s);
	}
	return 0;
}
