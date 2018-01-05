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

int touch(int, char *);
uint32_t now;

void
usage(void)
{
	fprint(2, "usage: touch [-c] [-t time] files\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *t, *s;
	int nocreate = 0;
	int status = 0;

	now = time(0);
	ARGBEGIN{
	case 't':
		t = EARGF(usage());
		now = strtoul(t, &s, 0);
		if(s == t || *s != '\0')
			usage();
		break;
	case 'c':
		nocreate = 1;
		break;
	default:
		usage();
	}ARGEND

	if(!*argv)
		usage();
	while(*argv)
		status += touch(nocreate, *argv++);
	if(status)
		exits("touch");
	exits(0);
}

int
touch(int nocreate, char *name)
{
	Dir stbuff;
	int fd;

	nulldir(&stbuff);
	stbuff.mtime = now;
	if(dirwstat(name, &stbuff) >= 0)
		return 0;
	if(nocreate){
		fprint(2, "touch: %s: cannot wstat: %r\n", name);
		return 1;
	}
	if((fd = create(name, OREAD|OEXCL, 0666)) < 0){
		fprint(2, "touch: %s: cannot create: %r\n", name);
		return 1;
	}
	dirfwstat(fd, &stbuff);
	close(fd);
	return 0;
}
