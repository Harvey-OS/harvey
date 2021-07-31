#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

int
cache(int fd)
{
	int i, p[2];
	char d[DIRLEN];
	char partition[2*NAMELEN];
	char bd[NAMELEN];

	if(stat("/cfs", d) < 0)
		return fd;

	readfile("#e/bootdisk", bd, sizeof(bd));
	if(*bd){
		/* damned artificial intelligence */
		i = strlen(bd);
		if(strcmp("disk", &bd[i-4]) == 0)
			bd[i-4] = 0;
		else if(strcmp("fs", &bd[i-2]) == 0)
			bd[i-2] = 0;
		sprint(partition, "%scache", bd);
		if(stat(partition, d) < 0)
			*bd = 0;
	}
	if(*bd == 0){
		sprint(partition, "%scache", bootdisk);
		if(stat(partition, d) < 0)
			return fd;
	}
	print("cfs...");
	if(pipe(p)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		close(p[1]);
		dup(fd, 0);
		close(fd);
		dup(p[0], 1);
		close(p[0]);
		if(fflag)
			execl("/cfs", "bootcfs", "-rs", "-f", partition, 0);
		else
			execl("/cfs", "bootcfs", "-s", "-f", partition, 0);
		break;
	default:
		close(p[0]);
		close(fd);
		fd = p[1];
		break;
	}
	return fd;
}
