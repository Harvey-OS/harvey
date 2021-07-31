#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

void configip(char* dev);

// HACK - take over from boot since files system is not
// available on a pipe

void
configsac(Method *mp)
{
	int fd;
	char cmd[64];
	char *t;

	USED(mp);

	//  create the name space, mount the root fs
	if(bind("/", "/", MREPL) < 0)
		fatal("bind /");
	if(bind("#C", "/", MAFTER) < 0)
		fatal("bind /");

	// yet another hack - figure out what to call ourselves
	t = getenv("terminal");
	if(t == nil)
		t = "brick";

	// fixed sysname - enables correct namespace file
	fd = open("#c/sysname", OWRITE);
	if(fd < 0)
		fatal("open sysname");
	write(fd, t, strlen(t));
	close(fd);

	fd = open("#c/hostowner", OWRITE);
	if(fd < 0)
		fatal("open sysname");
	write(fd, t, strlen(t));
	close(fd);

	sprint(cmd, "/%s/init", cputype);
	print("starting %s\n", cmd);
	execl(cmd, "init", "-c", 0);
	fatal(cmd);
}

int
authsac(void)
{
	// does not get here
	return -1;
}

int
connectsac(void)
{
	// does not get here
	return -1;
}
