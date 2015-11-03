/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
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

#define BUFSZ (32*1024)
void
main(void)
{
	int w;
	char msg[BUFSZ+1];
	print("This test write 32k 1 to stderr to test /dev/cons behaviour.\n");
	print("The test pass if the everything get written, including 'PASS'.\n");

	memset(msg, '1', BUFSZ);
	msg[BUFSZ] = 0;

	if((w = fprint(2, "%s", msg)) < BUFSZ){
		print("\nFAIL, written %d instead of %d\n", w, BUFSZ);
		exits("FAIL");
	}

	print("\nPASS\n");
	exits(0);
}
