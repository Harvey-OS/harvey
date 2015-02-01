/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Convert troff -ms input to HTML.
 */

#include "a.h"

Biobuf	bout;
char*	tmacdir;
int		verbose;
int		utf8 = 0;

void
usage(void)
{
	fprint(2, "usage: htmlroff [-iuv] [-m mac] [-r an] [file...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, dostdin;
	char *p;
	Rune *r;
	Rune buf[2];
	
	Binit(&bout, 1, OWRITE);
	fmtinstall('L', linefmt);
	quotefmtinstall();
	
	tmacdir = "/sys/lib/tmac";
	dostdin = 0;
	ARGBEGIN{
	case 'i':
		dostdin = 1;
		break;
	case 'm':
		r = erunesmprint("%s/tmac.%s", tmacdir, EARGF(usage()));
		if(queueinputfile(r) < 0)
			fprint(2, "%S: %r\n", r);
		break;
	case 'r':
		p = EARGF(usage());
		p += chartorune(buf, p);
		buf[1] = 0;
		_nr(buf, erunesmprint("%s", p+1));
		break;
	case 'u':
		utf8 = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND

	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-") == 0)
			queuestdin();
		else
			queueinputfile(erunesmprint("%s", argv[i]));
	}
	if(argc == 0 || dostdin)
		queuestdin();
	
	run();
	Bprint(&bout, "\n");
	Bterm(&bout);
	exits(nil);
}

