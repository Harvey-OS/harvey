#include "lib9.h"

int
pipe(int fd[2])
{
	if(bind("#|", "/mnt", MREPL) < 0)
		return -1;
	fd[0] = open("/mnt/data", ORDWR);
	fd[1] = open("/mnt/data1", ORDWR);
/*	unmount(0, "/mnt"); */
	return 0;	
}
