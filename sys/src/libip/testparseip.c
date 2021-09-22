/*
 * parseiptest - exercise parseip
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>

static void
process(Biobuf *in, char *inname)
{
	char *line;
	uchar ip[IPaddrlen];

	USED(inname);
	while((line = Brdline(in,'\n')) != nil) {
		line[Blinelen(in)-1] = '\0';
		memset(ip, 0, sizeof ip);
		if (parseip(ip, line) == -1)
			fprint(2, "can't parse %s\n", line);
		else
			print("parsed %s as %I\n", line, ip);
	}
}

static void
usage(void)
{
	fprint(2, "usage: %s [file]...\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i;
	char *sts = nil;
	Biobuf stdinb, stdoutb;
	Biobuf *in, *stdin = &stdinb, *stdout = &stdoutb;

	ARGBEGIN {
	default:
		usage();
		break;
	} ARGEND
	if (argc < 0)
		usage();

	fmtinstall('I', eipfmt);
	Binit(stdin,  0, OREAD);
	Binit(stdout, 1, OWRITE);

	if (argc == 0)
		process(stdin, "/fd/0");
	else
		for (i = 0; i < argc; i++) {
			in = Bopen(argv[i], OREAD);
			if (in == nil) {
				fprint(2, "%s: can't open %s: %r\n",
					argv0, argv[i]);
				sts = "open";
			} else {
				process(in, argv[i]);
				Bterm(in);
			}
		}
	exits(sts);
}
