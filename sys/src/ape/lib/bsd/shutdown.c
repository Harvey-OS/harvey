#include <sys/types.h>
#include <unistd.h>

int
shutdown(int fd, int how)
{
	if(how == 2)
		close(fd);

	return 0;
}
