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

void
Bpass(Biobuf *bin, Biobuf *bout, int n)
{
	char buf[8192];
	int m;

	while(n > 0) {
		m = sizeof buf;
		if(m > n)
			m = n;
		m = Bread(bin, buf, m);
		if(m <= 0) {
			fprint(2, "corrupt archive\n");
			exits("notdone");
		}
		Bwrite(bout, buf, m);
		n -= m;
	}
	assert(n == 0);
}

void
main(int argc, char **argv)
{
	char *p, *f[10];
	Biobuf bin, bout;
	int nf;
	uint32_t d, size;

	if(argc != 2) {
		fprint(2, "usage: cat mkfs-archive | touchfs date (in seconds)\n");
		exits("usage");
	}

	d = strtoul(argv[1], 0, 0);

	quotefmtinstall();
	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);

	while(p = Brdline(&bin, '\n')) {
		p[Blinelen(&bin)-1] = '\0';
		if(strcmp(p, "end of archive") == 0) {
			Bprint(&bout, "end of archive\n");
			exits(0);
		}

		nf = tokenize(p, f, nelem(f));
		if(nf != 6) {
			fprint(2, "corrupt archive\n");
			exits("notdone");
		}

		Bprint(&bout, "%q %q %q %q %lu %q\n",
			f[0], f[1], f[2], f[3], d, f[5]);

		size = strtoul(f[5], 0, 0);
		Bpass(&bin, &bout, size);
	}
	fprint(2, "premature end of archive\n");
	exits("notdone");
}
