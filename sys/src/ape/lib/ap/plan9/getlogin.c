#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/limits.h>

char *
getlogin(void)
{
	static char buf[NAME_MAX+1];
	int f, n;

	f = open("/dev/user", O_RDONLY);
	if(f < 0)
		return 0;
	n = read(f, buf, NAME_MAX+1);
	close(f);
	return (n>=0)? buf : 0;
}
