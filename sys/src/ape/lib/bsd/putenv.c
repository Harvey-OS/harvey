#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int
putenv(char *s)
{
	int f, n;
	char *value;
	char buf[300];

	value = strchr(s, '=');
	if (value) {
		n = value-s;
		if(n<=0 || n > sizeof(buf)-6)
			return -1;
		strcpy(buf, "/env/");
		strncpy(buf+5, s, n);
		buf[n+5] = 0;
		f = creat(buf, 0666);
		if(f < 0)
			return 1;
		value++;
		n = strlen(value);
		if(write(f, value, n) != n)
			return -1;
		close(f);
		return 0;
	} else
		return -1;
}
