/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <u.h>
#include <libc.h>

#include "console.h"

int linecontrol;
int blind;
int crnl;

static void
usage(void)
{
	fprint(2, "usage: %s [-d dbgfile] comfile program [args]\n", argv0);
	exits("usage");
}
static void
opencom(char *file)
{
	int fd;

	if((fd = open(file, ORDWR)) <= 0)
		sysfatal("open: %r");
	dup(fd, 0);
	dup(fd, 1);
	close(fd);
}
void
main(int argc, char *argv[])
{
	int fd, devmnt;

	blind = 0;
	linecontrol = 1;
	crnl = 1;
	ARGBEGIN{
	case 'd':
		enabledebug(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND;

	if(argc < 2)
		usage();

	opencom(argv[0]);

	fd = servecons(passthrough, passthrough, &devmnt);

	debug("%s %d: mounting cons for %s\n", argv0, getpid(), argv[0]);
	if(mount(fd, -1, "/dev", MBEFORE, "", devmnt) == -1)
		sysfatal("mount (%s): %r", argv[0]);

	debug("%s (%d): all services started, ready to exec(%s)\n", argv0, getpid(), argv[0]);

	/* become the requested program */
	rfork(RFNOTEG|RFREND|RFCFDG);

	fd = open("/dev/cons", OREAD);
	fd = open("/dev/cons", OWRITE);
	if(dup(fd, 2) != 2)
		sysfatal("bad FDs: %r");

	exec(argv[1], argv+1);
	sysfatal("exec %s: %r", argv[1]);
}
