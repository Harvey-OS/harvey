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
	fprint(2, "usage: %s [-d dbgfile] [-l] [-b] program [args]\n", argv0);
	exits("usage");
}
void
main(int argc, char *argv[])
{
	blind = 0;
	linecontrol = 1;
	crnl = 0;
	ARGBEGIN{
	case 'b':
		blind = 1;
		break;
	case 'd':
		enabledebug(EARGF(usage()));
		break;
	case 'l':
		linecontrol = 0;
		break;
	default:
		usage();
	}ARGEND;

	if(argc == 0)
		usage();

	servecons(argv, passthrough, passthrough);
}
