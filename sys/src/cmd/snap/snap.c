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
#include <bio.h>
#include "snap.h"

void
usage(void)
{
	fprint(2, "usage: %s [-o snapfile] pid...\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *user, *sys, *arch, *term, *ofile;
	int i;
	long pid, me;
	Biobuf *b;
	Dir *d;
	Proc *p;

	ofile = "/fd/1";
	ARGBEGIN
	{
	case 'o':
		ofile = ARGF();
		break;
	default:
		usage();
	}
	ARGEND;

	if(argc < 1)
		usage();

	/* get kernel compilation time */
	if((d = dirstat("#/")) == nil) {
		fprint(2, "cannot stat #/ ???\n");
		exits("stat");
	}

	if((b = Bopen(ofile, OWRITE)) == nil) {
		fprint(2, "cannot write to \"%s\"\n", ofile);
		exits("Bopen");
	}

	if((user = getuser()) == nil)
		user = "gre";
	if((sys = sysname()) == nil)
		sys = "gnot";
	if((arch = getenv("cputype")) == nil)
		arch = "unknown";
	if((term = getenv("terminal")) == nil)
		term = "unknown terminal type";

	Bprint(b, "process snapshot %ld %s@%s %s %ld \"%s\"\n",
	       time(0), user, sys, arch, d->mtime, term);
	me = getpid();
	for(i = 0; i < argc; i++) {
		if((pid = atol(argv[i])) == me)
			fprint(2, "warning: will not snapshot self\n");
		else if(p = snap(pid, 1))
			writesnap(b, p);
	}
	exits(0);
}
