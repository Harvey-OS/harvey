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

void
main(int argc, char **argv)
{
	char *dir;
	char *name;
	int i;

	dir = nil;
	ARGBEGIN{
	case 'd':
		if((dir=ARGF()) == nil)
			goto Usage;
		break;
	default:
		goto Usage;
	}ARGEND;

	if(argc < 1) {
	Usage:
		fprint(2, "usage: cleanname [-d pwd] name...\n");
		exits("usage");
	}

	for(i=0; i<argc; i++) {
		if(dir == nil || argv[i][0] == '/') {
			cleanname(argv[i]);
			print("%s\n", argv[i]);
		} else {
			name = malloc(strlen(argv[i])+1+strlen(dir)+1);
			if(name == nil) {
				fprint(2, "cleanname: out of memory\n");
				exits("out of memory");
			}
			sprint(name, "%s/%s", dir, argv[i]);
			cleanname(name);
			print("%s\n", name);
			free(name);
		}
	}
	exits(0);
}
