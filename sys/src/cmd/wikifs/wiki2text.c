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
#include <String.h>
#include <thread.h>
#include "wiki.h"

char *wikidir = ".";

void
usage(void)
{
	fprint(2, "usage: wiki2text [-d dir] wikifile\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	Biobuf *b;
	String *h;
	Whist *doc;

	ARGBEGIN{
	default:
		usage();
	case 'd':
		wikidir = EARGF(usage());
		break;
	}ARGEND

	if(argc != 1)
		usage();

	if((b = Bopen(argv[0], OREAD)) == nil)
		sysfatal("Bopen: %r");

	if((doc = Brdwhist(b)) == nil)
		sysfatal("Brdwtxt: %r");

	h = nil;
	for(i=0; i<doc->ndoc; i++){
		print("__________________ %d ______________\n", i);
		if((h = pagetext(s_reset(h), doc->doc[i].wtxt, 1)) == nil)
			sysfatal("wiki2html: %r");
		write(1, s_to_c(h), s_len(h));
	}
	exits(0);
}
