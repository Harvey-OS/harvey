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

#include "gconsole.h"

#define WAIT_FOR(v) while(rendezvous(&v, (void*)0x12345) == (void*)~0)
#define PROVIDE(v) while(rendezvous(&v, (void*)0x11111) == (void*)~0)

int stdio;
int rawmode;

/* read from input, write to output */
void
joint(char *name, int input, int output)
{
	int32_t pid, r, w;
	uint8_t buf[OutputBufferSize];

	pid = getpid();

	debug("%s %d started\n", name, pid);
	do
	{
		r = read(input, buf, OutputBufferSize);
		debug("%s %d: read(%d) returns %d\n", name, pid, input, r);
		if(r > 0){
			w = write(output, buf, r);
			buf[r] = 0;
			debug("%s %d: write(%d, buf, %d) returns %d\n", name, pid, output, r, w);
		}
	}
	while(r > 0 && w == r);

	close(input);
	debug("%s %d: close(%d)\n", name, pid, input);
	close(output);
	debug("%s %d: close(%d)\n", name, pid, output);

	debug("%s %d: shut down (r = %d, w = %d)\n", name, pid, r, w);
	if(r < 0)
		exits("read");
	if(w < 0)
		exits("write");
	if(w < r)
		exits("i/o error");
	exits(nil);
}

void
main(int argc, char *argv[])
{
	int mypid, fd, mnt[2], consreads[2], conswrites[2];

	ARGBEGIN{
	case 'd':
		enabledebug();
		break;
	case 's':
		stdio = 1;
		break;
	default:
		fprint(2, "usage: %s [-ds] program args\n", argv0);
		exits("usage");
	}ARGEND;

	if(argc == 0){
		fprint(2, "usage: %s [-ds] program args\n", argv0);
		exits("usage");
	}

	pipe(mnt);			/* mount pipe */
	pipe(consreads);	/* reads from /dev/cons */
	pipe(conswrites);	/* writes to /dev/cons */

	mypid = getpid();

	debug("%s %d: started\n", argv0, mypid);

	/* start output device writer */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("rfork (%s)", stdio ? "stdout writer" : "writecga");
			break;
		case 0:
			PROVIDE(conswrites);
			if(stdio) {
				joint("stdout writer", conswrites[1], 1);
			} else {
				if((fd = open("#P/cgamem", OWRITE)) == -1)
					sysfatal("open #P/cgamem: %r");
				writecga(conswrites[1], fd);
			}
			break;
		default:
			break;
	}

	/* start input device reader */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("rfork (%s)", stdio ? "stdin reader" : "readkeyboard");
			break;
		case 0:
			WAIT_FOR(conswrites);
			PROVIDE(consreads);
			if(stdio) {
				joint("stdin reader", 0, consreads[1]);
			} else {
				if((fd = open("#P/ps2keyb", OREAD)) == -1)
					sysfatal("open #P/ps2keyb: %r");
				readkeyboard(fd, consreads[1], conswrites[0]);
			}
			break;
		default:
			break;
	}

	/* start the file system */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("rfork (serve9p)");
			break;
		case 0:
			WAIT_FOR(consreads); /* implies also conswrites are available */
			PROVIDE(mnt);
			serve(mnt[1], consreads[0], conswrites[0]);
			break;
		default:
			break;
	}

	WAIT_FOR(mnt);

	debug("%s %d: all services started, ready to exec(%s)\n", argv0, mypid, argv[0]);

	/* become the requested program */
	rfork(RFNAMEG|RFNOTEG|RFREND);
	if(mount(mnt[0], -1, "/dev", MBEFORE, "", 'M') == -1)
		sysfatal("mount");

	/* clean fds */
	rfork(RFCFDG);

	fd = open("/dev/cons", OREAD);
	fd = open("/dev/cons", OWRITE);
	fd = dup(fd, 2);
	if(fd != 2)
		sysfatal("bad FDs");

	exec(argv[0], argv);
	sysfatal("exec (%s): %r", argv[0]);
}
