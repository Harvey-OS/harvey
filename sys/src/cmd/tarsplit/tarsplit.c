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
	vlong len;
	uvlong outoff = 0;
	static Hblock hdr;
	Hblock *hp = &hdr;

	while (getdir(hp, in, &len)) {
		if (outoff + Tblock + ROUNDUP((uvlong)len, Tblock) >
		    size - 2*Tblock)
			out = closeout(out, filenm, 1);
		if (out < 0)
			out = opennext(out, prefix);
		/* write directory block */
		outoff = writetar(out, (char *)hp, Tblock);
		passtar(hp, in, out, len);
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
