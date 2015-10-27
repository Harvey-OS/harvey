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

/* debugging */
int debugging;
extern void (*__assert)(char*);
static int
debugnotes(void *v, char *s)
{
	debug("%d: noted: %s\n", getpid(), s);
	return 0;
}
static void
traceassert(char*a)
{
	fprint(2, "assert failed: %s, %#p %#p %#p %#p %#p %#p %#p %#p %#p %#p\n", a,
		__builtin_return_address(2),
		__builtin_return_address(3),
		__builtin_return_address(4),
		__builtin_return_address(5),
		__builtin_return_address(6),
		__builtin_return_address(7),
		__builtin_return_address(8),
		__builtin_return_address(9),
		__builtin_return_address(10),
		__builtin_return_address(11)
	);
	exits(a);
}
void
enabledebug(const char *file)
{
	int fd;

	if (!debugging) {
		if((fd = open(file, OCEXEC|OTRUNC|OWRITE)) < 0){
			debug("open: %r\n");
			if((fd = create(file, OCEXEC|OWRITE, 0666)) < 0)
				sysfatal("create %r");
		}
		dup(fd, 2);
		close(fd);
		__assert = traceassert;
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

/* process management */
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
/* start the relevant services
 *
 * assumes that
 *  - fd 0 can be read by inputFilter
 *  - fd 1 can be written by outputFilter
 *  - fd 2 can receive debug info (if debugging)
 *
 * returns the fd to mount
 */
int
servecons(StreamFilter inputFilter, StreamFilter outputFilter, int *devmnt)
{
	int pid, input, output, fs, mnt;
	char *s;

	s = gethostowner();
	if(s == nil)
		sysfatal("cannot read hostowner");

	pid = getpid();

	if(!debugging)
		close(2);

	rfork(RFNAMEG);

	debug("%s %d: started, linecontrol = %d, blind = %d\n", argv0, pid, linecontrol, blind);

	fs = fsinit(&mnt, devmnt);

	/* start the file system */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFCENVG|RFNOTEG|RFCNAMEG|RFFDG)){
		case -1:
			sysfatal("rfork (file server)");
			break;
		case 0:
			close(0);
			close(1);
			close(mnt);
			s = strdup(s);
			PROVIDE(fs);
			rfork(RFREND);
			fsserve(fs, s);
			break;
		default:
			break;
	}

	WAIT_FOR(fs);
	free(s);
	s = nil;
	close(fs);

	/* start output device writer */
	debug("%s %d: starting output device writer\n", argv0, pid);
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFNOTEG|RFFDG|RFNAMEG)){
		case -1:
			sysfatal("rfork (output writer): %r");
			break;
		case 0:
			if(mount(mnt, -1, "/dev", MBEFORE, "", *devmnt) == -1)
				sysfatal("mount (output writer): %r");
			if((output = open("/dev/gconsout", OREAD)) == -1)
				sysfatal("open /dev/gconsout: %r");
			close(0);
			PROVIDE(output);
			rfork(RFCENVG|RFCNAMEG|RFREND|RFNOMNT);
			outputFilter(output, 1);
			break;
		default:
			break;
	}

	/* start input device reader */
	debug("%s %d: starting input device reader\n", argv0, pid);
	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFNOTEG|RFFDG|RFNAMEG)){
		case -1:
			sysfatal("rfork (input reader): %r");
			break;
		case 0:
			if(mount(mnt, -1, "/dev", MBEFORE, "", *devmnt) == -1)
				sysfatal("mount (input reader): %r");
			if((input = open("/dev/gconsin", OWRITE)) == -1)
				sysfatal("open /dev/gconsin: %r");
			close(1);
			PROVIDE(input);
			rfork(RFCENVG|RFCNAMEG|RFREND|RFNOMNT);
			inputFilter(0, input);
			break;
		default:
			break;
	}

	WAIT_FOR(input);
	WAIT_FOR(output);
	close(0);
	close(1);

	return mnt;
}
