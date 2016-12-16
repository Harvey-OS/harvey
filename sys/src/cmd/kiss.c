/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

static char line[512];
static char *argv[512];
static int argc;

void
main(int _, char *__)
{
	int i, amt;
	int fd = open("#P/cons", ORDWR);
	if (fd < 0)
		exits("fuck");
	if (fd != 0)
		dup(fd, 0);
	dup(fd, 1);
	dup(fd, 2);

	argv0 = "kiss";
	i = 0;
	argv[0] = line; // invariant. 
	while (1) {
		if (amt = read(0, &line[i], 1) < 1) {
			print("read: %r\n");
			break;
		}
		print("READ '%c' AT POS %d\n", line[i], i);
		if (line[i] == '\n') {
			line[i] = 0;
			print("LINE is %s\n", line);
			print("argv[0] is %s\n", argv[0]);
			switch(fork()) {
			default:
				wait();
				break;
			case 0:
				exec(argv[0], argv);
				exits("exec failed");
				break;
			case -1:
				print("rfork failed: %r\n");
				break;
			}
			i = 0;
			argc = 0;
			memset(argv, 0, sizeof(argv));
			memset(line, 0, sizeof(line));
			argv[0] = line;
		} else if (line[i] == ' ') {
				line[i] = 0;
				argc++, i++;
				argv[argc] = &line[i];
		} else {
			i++;
		}
		
	}
	exits(0);
}

