#include <u.h>
#include <libc.h>
#include <auth.h>

char *namespace;

void
usage(void)
{
	fprint(2, "usage: auth/none [-n namespace] [cmd ...]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char cmd[256];
	int fd;

	ARGBEGIN{
	case 'n':
		namespace = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if (rfork(RFENVG|RFNAMEG) < 0)
		sysfatal("can't make new pgrp");

	fd = open("#c/user", OWRITE);
	if (fd < 0)
		sysfatal("can't open #c/user");
	if (write(fd, "none", strlen("none")) < 0)
		sysfatal("can't become none");
	close(fd);

	if (newns("none", namespace) < 0)
		sysfatal("can't build namespace");

	if (argc > 0) {
		strecpy(cmd, cmd+sizeof cmd, argv[0]);
		exec(cmd, &argv[0]);
		if (strncmp(cmd, "/", 1) != 0
		&& strncmp(cmd, "./", 2) != 0
		&& strncmp(cmd, "../", 3) != 0) {
			snprint(cmd, sizeof cmd, "/bin/%s", argv[0]);
			exec(cmd, &argv[0]);
		}
	} else {
		strcpy(cmd, "/bin/rc");
		execl(cmd, cmd, nil);
	}
	sysfatal(cmd);
}
