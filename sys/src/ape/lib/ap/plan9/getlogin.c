#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/limits.h>

char *
getlogin_r(char *buf, int len)
{
	int f, n;

	f = open("/dev/user", O_RDONLY);
	if(f < 0)
		return 0;
	n = read(f, buf, len);
	buf[len-1] = 0;
	close(f);
	return (n>=0)? buf : 0;
}

char *
getlogin(void)
{
	static char buf[NAME_MAX+1];

	return getlogin_r(buf, sizeof buf);
}
