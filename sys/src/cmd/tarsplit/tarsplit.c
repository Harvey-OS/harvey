/*
 * tarsplit [-d] [-p pfx] [-s size] - split a tar archive into independent
 *	tar archives under some size
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "tar.h"

enum {
	Stdin,
	Endsize	= 2 * Tblock,		/* size of zero blocks at archive end */
};

/* private data */
static char *filenm;
static char *prefix = "ts.";
static vlong size = 512*1024*1024;	/* fits on a CD with room to spare */

static int
opennext(int out, char *prefix)
{
	static int filenum = 0;

	if (out >= 0) {
		fprint(2, "%s: opennext called with file open\n", argv0);
		exits("open botch");
	}
	free(filenm);
	filenm = smprint("%s%.5d", prefix, filenum++);
	fprint(2, "%s: %s ...", filenm, thisnm);
	out = create(filenm, OWRITE, 0666);
	if (out < 0)
		sysfatal("%s: %r", filenm);
	newarch();
	return out;
}

static int
split(int in, int out, char * /* inname */)
{
	vlong len, membsz;
	uvlong outoff = 0;
	static Hblock hdr;
	Hblock *hp = &hdr;

	while (getdir(hp, in, &len)) {
		membsz = Tblock + ROUNDUP((uvlong)len, Tblock);
		if (outoff + membsz + Endsize > size) {	/* won't fit? */
			out = closeout(out, filenm, 1);
			if (membsz + Endsize > size)
				sysfatal("archive member %s (%,lld) + overhead "
					"exceeds size limit %,lld", hp->name,
					len, size);
		}
		if (out < 0)
			out = opennext(out, prefix);
		/* write directory block */
		writetar(out, (char *)hp, Tblock);
		outoff = passtar(hp, in, out, len);
	}
	return out;
}

void
usage(void)
{
	fprint(2, "usage: %s [-p pfx] [-s size] [file]...\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int out = -1;

	ARGBEGIN {
	case 'p':
		prefix = EARGF(usage());
		break;
	case 's':
		size = atoll(EARGF(usage()));
		if (size < Tblock + Endsize)
			sysfatal("implausible max size of %lld bytes", size);
		break;
	default:
		usage();
	} ARGEND

	if (argc <= 0)
		out = split(Stdin, out, "/fd/0");
	else
		for (; argc-- > 0; argv++) {
			int in = open(argv[0], OREAD);

			if (in < 0)
				sysfatal("%s: %r", argv[0]);
			out = split(in, out, argv[0]);
			close(in);
		}
	closeout(out, filenm, 1);
	exits(0);
}
