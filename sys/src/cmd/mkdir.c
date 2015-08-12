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

char* e;
uint32_t mode = 0777L;

void
usage(void)
{
	fprint(2, "usage: mkdir [-p] [-m mode] dir...\n");
	exits("usage");
}

int
makedir(char* s)
{
	int f;

	if(access(s, AEXIST) == 0) {
		fprint(2, "mkdir: %s already exists\n", s);
		e = "error";
		return -1;
	}
	f = create(s, OREAD, DMDIR | mode);
	if(f < 0) {
		fprint(2, "mkdir: can't create %s: %r\n", s);
		e = "error";
		return -1;
	}
	close(f);
	return 0;
}

void
mkdirp(char* s)
{
	char* p;

	for(p = strchr(s + 1, '/'); p; p = strchr(p + 1, '/')) {
		*p = 0;
		if(access(s, AEXIST) != 0 && makedir(s) < 0)
			return;
		*p = '/';
	}
	if(access(s, AEXIST) != 0)
		makedir(s);
}

void
main(int argc, char* argv[])
{
	int i, pflag;
	char* m;

	pflag = 0;
	ARGBEGIN
	{
	default:
		usage();
	case 'm':
		m = ARGF();
		if(m == nil)
			usage();
		mode = strtoul(m, &m, 8);
		if(mode > 0777)
			usage();
		break;
	case 'p':
		pflag = 1;
		break;
	}
	ARGEND

	for(i = 0; i < argc; i++) {
		if(pflag)
			mkdirp(argv[i]);
		else
			makedir(argv[i]);
	}
	exits(e);
}
