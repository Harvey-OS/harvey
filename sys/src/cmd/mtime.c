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
usage(void)
{
	fprint(2, "usage: mtime file...\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int errors, i;
	Dir *d;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	errors = 0;
	for(i=0; i<argc; i++){
		if((d = dirstat(argv[i])) == nil){
			fprint(2, "stat %s: %r\n", argv[i]);
			errors = 1;
		}else{
			print("%11lud %s\n", d->mtime, argv[i]);
			free(d);
		}
	}
	exits(errors ? "errors" : nil);
}
