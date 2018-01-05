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

uint64_t	count[Runemax+1];
Biobuf	bout;

void	usage(void);
void	freq(int, char*);
int32_t	flag;
enum
{
	Fdec	= 1<<0,
	Fhex	= 1<<1,
	Foct	= 1<<2,
	Fchar	= 1<<3,
	Frune	= 1<<4,
};

void
main(int argc, char *argv[])
{
	int f, i;

	flag = 0;
	Binit(&bout, 1, OWRITE);
	ARGBEGIN{
	case 'd':
		flag |= Fdec;
		break;
	case 'x':
		flag |= Fhex;
		break;
	case 'o':
		flag |= Foct;
		break;
	case 'c':
		flag |= Fchar;
		break;
	case 'r':
		flag |= Frune;
		break;
	default:
		usage();
	}ARGEND
	if((flag&(Fdec|Fhex|Foct|Fchar)) == 0)
		flag |= Fdec | Fhex | Foct | Fchar;
	if(argc < 1) {
		freq(0, "-");
		exits(0);
	}
	for(i=0; i<argc; i++) {
		f = open(argv[i], 0);
		if(f < 0) {
			fprint(2, "open %s: %r\n", argv[i]);
			continue;
		}
		freq(f, argv[i]);
		close(f);
	}
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: freq [-cdorx] [file ...]\n");
	exits("usage");
}

void
freq(int f, char *s)
{
	Biobuf bin;
	int32_t c, i;

	memset(count, 0, sizeof(count));
	Binit(&bin, f, OREAD);
	if(flag & Frune) {
		for(;;) {
			c = Bgetrune(&bin);
			if(c < 0)
				break;
			count[c]++;
		}
	} else {
		for(;;) {
			c = Bgetc(&bin);
			if(c < 0)
				break;
			count[c]++;
		}
	}
	Bterm(&bin);
	if(c != Beof)
		fprint(2, "freq: read error on %s\n", s);

	for(i=0; i<nelem(count); i++) {
		if(count[i] == 0)
			continue;
		if(flag & Fdec)
			Bprint(&bout, "%3ld ", i);
		if(flag & Foct)
			Bprint(&bout, "%.3lo ", i);
		if(flag & Fhex)
			Bprint(&bout, "%.2lx ", i);
		if(flag & Fchar) {
			if(i <= 0x20 ||
			   i >= 0x7f && i < 0xa0 ||
			   i > 0xff && !(flag & Frune))
				Bprint(&bout, "- ");
			else
				Bprint(&bout, "%C ", (int)i);
		}
		Bprint(&bout, "%8llu\n", count[i]);
	}
	Bflush(&bout);
}
