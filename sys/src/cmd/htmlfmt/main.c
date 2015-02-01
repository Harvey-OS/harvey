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
#include <draw.h>
#include <html.h>
#include "dat.h"

char *url = "";
int aflag;
int width = 70;
int defcharset;

void
usage(void)
{
	fprint(2, "usage: htmlfmt [-c charset] [-u URL] [-a] [-l length] [file ...]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i, fd;
	char *p, *err, *file;
	char errbuf[ERRMAX];

	ARGBEGIN{
	case 'a':
		aflag++;
		break;
	case 'c':
		p = smprint("<meta charset=\"%s\">", EARGF(usage()));
		defcharset = charset(p);
		free(p);
		break;
	case 'l': case 'w':
		err = EARGF(usage());
		width = atoi(err);
		if(width <= 0)
			usage();
		break;
	case 'u':
		url = EARGF(usage());
		aflag++;
		break;
	default:
		usage();
	}ARGEND

	err = nil;
	file = "<stdin>";
	if(argc == 0)
		err = loadhtml(0);
	else
		for(i=0; err==nil && i<argc; i++){
			file = argv[i];
			fd = open(file, OREAD);
			if(fd < 0){
				errstr(errbuf, sizeof errbuf);
				err = errbuf;
				break;
			}
			err = loadhtml(fd);
			close(fd);
			if(err)
				break;
		}
	if(err)
		fprint(2, "htmlfmt: processing %s: %s\n", file, err);
	exits(err);
}
