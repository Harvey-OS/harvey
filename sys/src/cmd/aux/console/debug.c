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
#include <fcall.h>

#include "console.h"

static int enabled;

int
debugnotes(void *v, char *s)
{
	debug("%d: noted: %s\n", getpid(), s);
	return 0;
}

void
enabledebug(void)
{
	if (!enabled) {
		if(!atnotify(debugnotes, 1)){
			fprint(2, "atnotify: %r\n");
			exits("atnotify");
		}
		fmtinstall('F', fcallfmt);
	}
	++enabled;
}

void
debug(const char *fmt, ...)
{
	va_list arg;

	if (enabled) {
		va_start(arg, fmt);
		vfprint(2, fmt, arg);
		va_end(arg);
	}
}
