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

int blind;
int linecontrol;
int crnl;

static void
usage(void)
{
	fprint(2, "usage: %s [-d dbgfile] srvname\n", argv0);
	exits("usage");
}
static void
post(char *srv, int fd)
{
	int f;
	char buf[128];

	fprint(2, "post %s...\n", srv);
	sprint(buf, "#s/%s", srv);
	f = create(buf, OWRITE, 0666);
	if(f < 0)
		sysfatal("create(%s)", srv);
	sprint(buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		sysfatal("write");
}
void
main(int argc, char *argv[])
{
	int fd, devmnt;
	char *srv;

	systemwide = 1; /* only screenconsole allows multiple attach */
	blind = 0;
	linecontrol = 1;
	crnl = 0;
	ARGBEGIN{
	case 'd':
		enabledebug(EARGF(usage()));
		break;
	default:
		usage();
		break;
	}ARGEND;

	if(argc > 1)
		usage();
	if(argc)
		srv = argv[0];
	else
		srv = "screenconsole";

	/* first try in /dev so that binding can work */
	if((fd = open("/dev/ps2keyb", OREAD)) <= 0)
		if((fd = open("#P/ps2keyb", OREAD)) <= 0)
			sysfatal("open keyboard: %r");
	dup(fd, 0);
	close(fd);
	if((fd = open("/dev/cgamem", OWRITE)) <= 0)
		if((fd = open("#P/cgamem", OWRITE)) <= 0)
			sysfatal("open screen: %r");
	dup(fd, 1);
	close(fd);

	fd = servecons(readkeyboard, writecga, &devmnt);
	post(srv, fd);
	exits(0);
}
