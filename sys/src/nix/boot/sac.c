#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

/*
 * HACK - take over from boot since file system is not
 * available on a pipe
 */

void
configsac(Method *mp)
{
	int fd;
	char cmd[64];

	USED(mp);

	/*
	 *  create the name space, mount the root fs
	 */
	if(bind("/", "/", MREPL) < 0)
		fatal("bind /");
	if(bind("#C", "/", MAFTER) < 0)
		fatal("bind /");

	/* fixed sysname - enables correct namespace file */
	fd = open("#c/sysname", OWRITE);
	if(fd < 0)
		fatal("open sysname");
	write(fd, "brick", 5);
	close(fd);

	fd = open("#c/hostowner", OWRITE);
	if(fd < 0)
		fatal("open sysname");
	write(fd, "brick", 5);
	close(fd);

	sprint(cmd, "/%s/init", cputype);
	print("starting %s\n", cmd);
	execl(cmd, "init", "-c", 0);
	fatal(cmd);
}

int
connectsac(void)
{
	/* does not get here */
	return -1;
}
