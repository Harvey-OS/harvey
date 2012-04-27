#include	<u.h>
#include	<libc.h>

int
ziop(int fd[2])
{
	if(bind("#∏", "/mnt/zp", MREPL|MCREATE) < 0)
		return -1;
	fd[0] = open("/mnt/zp/data", ORDWR);
	if(fd[0] < 0)
		return -1;
	fd[1] = open("/mnt/zp/data1", ORDWR);
	if(fd[1] < 0){
		close(fd[0]);
		return -1;
	}
	return 0;
}
