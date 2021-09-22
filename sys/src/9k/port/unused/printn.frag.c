#pragma varargck argpos printn 3

/* print first n times; avoid flooding console */
void
printn(int n, int *cntr, char *fmt, ...)
{
	if (*cntr < n) {
		va_list arg;
		char buf[PRINTSIZE];
	
		++*cntr;
		va_start(arg, fmt);
		vseprint(buf, buf+sizeof(buf), fmt, arg);
		va_end(arg);
		print("%s", buf);
	}
}

