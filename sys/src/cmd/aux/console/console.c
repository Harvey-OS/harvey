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

static void
usage(void)
{
	fprint(2, "usage: %s [-d dbgfile] [-l] [-b] [-f comfile] program [args]\n", argv0);
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
	char *f;

	blind = 0;
	linecontrol = 1;
	ARGBEGIN{
	case 'b':
		blind = 1;
		break;
	case 'd':
		enabledebug(EARGF(usage()));
		break;
	case 'f':
		f = EARGF(usage());
		break;
	case 'l':
		linecontrol = 0;
		break;
	default:
		usage();
	}ARGEND;

	if(argc == 0)
		usage();

	if(f != nil)
		opencom(f);

	servecons(argv, passthrough, passthrough);
}
