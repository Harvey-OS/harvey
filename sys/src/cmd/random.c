/*
 * random - print a random number or string
 */

#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>

static int string, prchars, low, high;

static void
usage(void)
{
	fprint(2, "usage: %s [-s nch]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *buf, *out;

	ARGBEGIN {
	case 's':
		++string;
		prchars = atoi(EARGF(usage()));
		break;
	default:
		usage();
		break;
	} ARGEND

	if (!string) {
		print("%f\n", (double)ntruerand(~0ul) / (1ull<<32));
		exits(0);
	}

	/* fill buf from /dev/random */
	buf = malloc(prchars + 1);
	if (buf == nil)
		sysfatal("out of memory: %r");
	genrandom((uchar *)buf, prchars);
	buf[prchars] = '\0';

	/* encode as printable (base64) and print truncated */
	fmtinstall('[', encodefmt);
	out = smprint("%.*[", prchars, buf);
	if (out == nil)
		sysfatal("out of memory: %r");
	print("%.*s\n", prchars, out);
	exits(0);
}
