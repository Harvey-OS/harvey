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
#include <draw.h>
#include <memdraw.h>
#include <ctype.h>
#include <bio.h>
#include <flate.h>
#include "imagefile.h"

void
usage(void)
{
	fprint(2, "usage: topng [-c 'comment'] [-g 'gamma'] [file]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Biobuf bout;
	Memimage *i;
	int fd;
	char *err, *filename;
	ImageInfo II;

	ARGBEGIN{
	case 'c':
		II.comment = ARGF();
		if(II.comment == nil)
			usage();
		II.fields_set |= II_COMMENT;
		break;
	case 'g':
		II.gamma = atof(ARGF());
		if(II.gamma == 0.)
			usage();
		II.fields_set |= II_GAMMA;
		break;
	case 't':
		break;
	default:
		usage();
	}ARGEND

	if(Binit(&bout, 1, OWRITE) < 0)
		sysfatal("Binit failed: %r");
	memimageinit();

	if(argc == 0){
		fd = 0;
		filename = "<stdin>";
	}else{
		fd = open(argv[0], OREAD);
		if(fd < 0)
			sysfatal("can't open %s: %r", argv[0]);
		filename = argv[0];
	}

	i = readmemimage(fd);
	if(i == nil)
		sysfatal("can't readimage %s: %r", filename);
	close(fd);

	err = memwritepng(&bout, i, &II);
	freememimage(i);

	if(err != nil)
		fprint(2, "topng: %s\n", err);
	exits(err);
}
