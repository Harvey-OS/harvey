#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

int
cache(int fd)
{
	int argc, i, p[2];
	char *argv[5], bd[NAMELEN], buf[256], d[DIRLEN], partition[2*NAMELEN], *pp;

	if(stat("/cfs", d) < 0)
		return fd;

	*partition = 0;

	readfile("#e/cfs", buf, sizeof(buf));
	if(*buf){
		argc = getfields(buf, argv, 4, 1, " ");
		for(i = 0; i < argc; i++){
			if(strcmp(argv[i], "off") == 0)
				return fd;
			else if(stat(argv[i], d) >= 0){
				strncpy(partition, argv[i], sizeof(partition)-1);
				partition[sizeof(partition)-1] = 0;
			}
		}
	}

	if(*partition == 0){
		readfile("#e/bootdisk", bd, sizeof(bd));
		if(*bd){
			if(pp = strchr(bd, ':'))
				*pp = 0;
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
