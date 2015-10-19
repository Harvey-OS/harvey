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

#define WAIT_FOR(v) while(rendezvous(&v, (void*)0x12345) == (void*)~0)
#define PROVIDE(v) while(rendezvous(&v, (void*)0x11111) == (void*)~0)

int stdio;
int blind;

static int debugging;
static int
debugnotes(void *v, char *s)
{
	debug("%d: noted: %s\n", getpid(), s);
	return 0;
}
static void
enabledebug(void)
{
	if (!debugging) {
		if(!atnotify(debugnotes, 1)){
			fprint(2, "atnotify: %r\n");
			exits("atnotify");
		}
		fmtinstall('F', fcallfmt);
	}
	++debugging;
}
void
debug(const char *fmt, ...)
{
	va_list arg;

	if (debugging) {
		va_start(arg, fmt);
		vfprint(2, fmt, arg);
		va_end(arg);
	}
}

static char*
gethostowner(void)
{
	int f, r;
	char *res;

	res = (char*)malloc(256);
	if(res == nil)
		sysfatal("out of memory");
	f = open("#c/hostowner", OREAD);
	if(f < 0)
		sysfatal("open(#c/hostowner)");
	r = read(f, res, 255);
	if(r < 0)
		sysfatal("read(#c/hostowner)");
	close(f);
	return res;
}

/* read from input, write to output */
void
joint(char *name, int input, int output)
{
	int32_t pid, r, w;
	uint8_t *buf;

	pid = getpid();
	buf = (uint8_t*)malloc(IODATASZ);

	debug("%s %d started\n", name, pid);
	do
	{
		w = 0;
		r = read(input, buf, IODATASZ);
		debug("%s %d: read(%d) returns %d\n", name, pid, input, r);
		if(r > 0){
			w = write(output, buf, r);
			debug("%s %d: write(%d, buf, %d) returns %d\n", name, pid, output, r, w);
		}
		debug("%s %d: r = %d, w = %d\n", name, pid, r, w);
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
	int mypid, input, output, fs, mnt, devmnt;
	char *owner;

	blind = 0;
	ARGBEGIN{
	case 'd':
		enabledebug();
		break;
	case 's':
		stdio = 1;
		break;
	case 'b':
		blind = 1;
		stdio = 1;
		break;
	default:
		fprint(2, "usage: %s [-bds] program args\n", argv0);
		exits("usage");
	}ARGEND;

	if(argc == 0){
		fprint(2, "usage: %s [-bds] program args\n", argv0);
		exits("usage");
	}

	owner = gethostowner();
	if(owner == nil)
		sysfatal("cannot read hostowner");

	mypid = getpid();
	if(!debugging)
		close(2);

	debug("%s %d: started, stdio = %d, blind = %d, owner = %s\n", argv0, mypid, stdio, blind, owner);

	rfork(RFNAMEG);

	fs = fsinit(&mnt, &devmnt);

	/* start the file system */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFCENVG|RFNOTEG|RFCNAMEG|RFFDG)){
		case -1:
			sysfatal("rfork (file server)");
			break;
		case 0:
			close(0);
			close(1);
			close(mnt);
			PROVIDE(fs);
			rfork(RFREND);
			fsserve(fs, owner);
			free(owner);	/* we share memory, thus free too */
			owner = nil;
			break;
		default:
			close(fs);
			break;
	}

	WAIT_FOR(fs);

	/* start output device writer */
	if(stdio)
		output = 1;
	else {
		close(1);
		if((output = open("#P/cgamem", OWRITE)) == -1)
			sysfatal("open #P/cgamem: %r");
	}
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFNOTEG|RFFDG|RFNAMEG)){
		case -1:
			sysfatal("rfork (%s)", stdio ? "stdout writer" : "writecga");
			break;
		case 0:
			if(mount(mnt, -1, "/dev", MBEFORE, "", devmnt) == -1)
				sysfatal("mount (%s)", stdio ? "stdout writer" : "writecga");
			if((input = open("/dev/gconsout", OREAD)) == -1)
				sysfatal("open /dev/gconsout: %r");
			PROVIDE(output);
			rfork(RFCENVG|RFCNAMEG|RFREND|RFNOMNT);
			if(stdio)
				joint("stdout writer", input, output);
			else
				writecga(input, output);
			break;
		default:
			close(output);
			break;
	}

	/* start input device reader */
	if(stdio)
		input = 0;
	else{
		close(0);
		if((input = open("#P/ps2keyb", OREAD)) == -1)
			sysfatal("open #P/ps2keyb: %r");
	}
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFNOTEG|RFFDG|RFNAMEG)){
		case -1:
			sysfatal("rfork (%s)", stdio ? "stdin reader" : "readkeyboard");
			break;
		case 0:
			if(mount(mnt, -1, "/dev", MBEFORE, "", devmnt) == -1)
				sysfatal("mount (%s)", stdio ? "stdin reader" : "readkeyboard");
			if((output = open("/dev/gconsin", OWRITE)) == -1)
				sysfatal("open /dev/gconsin: %r");
			WAIT_FOR(output);
			PROVIDE(input);
			rfork(RFCENVG|RFCNAMEG|RFREND|RFNOMNT);
			if(stdio)
				joint("stdin reader", input, output);
			else
				readkeyboard(input, output);
			break;
		default:
			close(input);
			break;
	}

	WAIT_FOR(input);	/* means that also output is available */

	putenv("prompt", "gconsole$ ");
	debug("%s %d: all services started, ready to exec(%s)\n", argv0, mypid, argv[0]);

	/* become the requested program */
	if(mount(mnt, -1, "/dev", MBEFORE, "", devmnt) == -1)
		sysfatal("mount (%s)", stdio ? "stdin reader" : "readkeyboard");

	rfork(RFNOTEG|RFREND|RFCFDG);

	input = open("/dev/cons", OREAD);
	output = open("/dev/cons", OWRITE);
	if(dup(output, 2) != 2)
		sysfatal("bad FDs");

	exec(argv[0], argv);
	sysfatal("exec (%s): %r", argv[0]);
}
