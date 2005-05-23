#include <u.h>
#include <libc.h>
#include <auth.h>

void
main(int argc, char *argv[])
{
	char cmd[256];
	int fd;

	argv0 = argv[0];
	if (rfork(RFENVG|RFNAMEG) < 0)
		sysfatal("can't make new pgrp");

	fd = open("#c/user", OWRITE);
	if (fd < 0)
		sysfatal("can't open #c/user");
	if (write(fd, "none", strlen("none")) < 0)
		sysfatal("can't become none");
	close(fd);

	if (newns("none", nil) < 0)
		sysfatal("can't build namespace");

	if (argc > 1) {
		strecpy(cmd, cmd+sizeof cmd, argv[1]);
		exec(cmd, &argv[1]);
		if (strncmp(cmd, "/", 1) != 0
		&& strncmp(cmd, "./", 2) != 0
		&& strncmp(cmd, "../", 3) != 0) {
			snprint(cmd, sizeof cmd, "/bin/%s", argv[1]);
			exec(cmd, &argv[1]);
		}
	} else {
		strcpy(cmd, "/bin/rc");
		execl(cmd, cmd, nil);
	}
	sysfatal(cmd);
}
