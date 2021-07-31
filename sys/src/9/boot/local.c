#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

static char *disk;

void
configlocal(Method *mp)
{
	if(*sys == '/' || *sys == '#')
		disk = sys;
	else
		disk = mp->arg;
	USED(mp);
}

int
authlocal(void)
{
	return -1;
}

int
connectlocal(void)
{
	int p[2];
	char d[DIRLEN];
	char partition[2*NAMELEN];
	char *dev;

	if(stat("/fs", d) < 0)
		return -1;

	dev = disk ? disk : bootdisk;
	sprint(partition, "%sfs", dev);
	if(stat(partition, d) < 0){
		strcpy(partition, dev);
		if(stat(partition, d) < 0)
			return -1;
	}

	print("fs...");
	if(bind("#c", "/dev", MREPL) < 0)
		fatal("bind #c");
	if(bind("#p", "/proc", MREPL) < 0)
		fatal("bind #p");
	if(pipe(p)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		dup(p[0], 0);
		dup(p[1], 1);
		close(p[0]);
		close(p[1]);
		execl("/fs", "fs", "-f", partition, "-s", 0);
		fatal("can't exec fs");
	default:
		break;
	}

	close(p[1]);
	return p[0];
}
