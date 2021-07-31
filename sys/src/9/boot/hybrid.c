#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

extern char *timeserver;
static Method *config;

static void
confighyb(Method *mp, void (*cf)(Method*))
{
	char *save;

	config = mp;
	save = mp->arg;
	mp->arg = 0;
	(*cf)(mp);
	mp->arg = save;
	configlocal(mp);
}

void
confighybrid(Method *mp)
{
	confighyb(mp, config9600);
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
	doauthenticate(fd, config);
	/*if(cfs)
		fd = (*cfs)(fd);*/
	srvcreate("bootes", fd);
	timeserver = "#s/bootes";
	return connectlocal();
}

void
configHybrid(Method *mp)
{
	confighyb(mp, config19200);
}

int
authHybrid(void)
{
	return dkauth();
}

int
connectHybrid(void)
{
	return connecthybrid();
}
