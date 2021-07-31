#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

extern char *timeserver;

void
confighybrid(Method *mp)
{
	char *save;

	save = mp->arg;
	mp->arg = 0;
	config9600(mp);
	mp->arg = save;
	configlocal(mp);
}

int
authhybrid(void)
{
	return dkauth();
}

int
connecthybrid(void)
{
	int fd;

	fd = dkconnect();
	if(fd < 0)
		fatal("can't connect to file server");
	nop(fd);
	session(fd);
	/*if(cfs)
		fd = (*cfs)(fd);*/
	srvcreate("bootes", fd);
	timeserver = "#s/bootes";
	return connectlocal();
}
