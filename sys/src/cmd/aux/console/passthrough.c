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

/* read from input, write to output */
void
passthrough(int input, int output)
{
	int32_t pid, r, w;
	uint8_t *buf;
	char *name;

	name = smprint("coping %d to %d", input, output);

	pid = getpid();
	buf = (uint8_t*)malloc(IODATASZ);

	debug("%s (%d): started\n", name, pid);
	do
	{
		w = 0;
		r = read(input, buf, IODATASZ);
		//debug("%s (%d): read(%d) returns %d\n", name, pid, input, r);
		if(r > 0){
			w = write(output, buf, r);
			//debug("%s (%d): write(%d, buf, %d) returns %d\n", name, pid, output, r, w);
		}
		//debug("%s (%d): r = %d, w = %d\n", name, pid, r, w);
	}
	while(r > 0 && w == r);

	close(input);
	debug("%s (%d): close(%d)\n", name, pid, input);
	close(output);
	debug("%s (%d): close(%d)\n", name, pid, output);

	debug("%s (%d) shut down (r = %d, w = %d)\n", name, pid, r, w);
	if(r < 0)
		exits("read");
	if(w < 0)
		exits("write");
	if(w < r)
		exits("i/o error");
	exits(nil);
}
