/* regression tests for chartorune and runetochar */
#include <u.h>
#include <libc.h>

typedef struct Runetest Runetest;

struct Runetest {
	Rune	rune;
	char	*utf;
};

Runetest runes[] = {
	0x000061, "\x61",		/* a */
	0x0003c0, "\xcf\x80",		/* π */
	0x00263a, "\xe2\x98\xba",	/* ☺ */
	0x0030c0, "\xe3\x83\x80",	/* ダ */
	0x00fbe9, "\xef\xaf\xa9",	/* ﯩ */
#ifdef UNICODE4
#endif
};

void
main(void)
{
	int n, utflen;
	char buf[UTFmax+1];
	Rune r;
	Runetest *rp;

	for (rp = runes; rp < runes + nelem(runes); rp++) {
		utflen = strlen(rp->utf);
		print("rune %#08lux %C %s %d byte(s) of utf:",
			(long)rp->rune, rp->rune, rp->utf, utflen);
		for (n = 0; n < utflen; n++)
			print(" %04#ux", (uchar)rp->utf[n]);

		n = chartorune(&r, rp->utf);
		if (n != utflen)
			print(", chartorune => %d for %d bytes", n, utflen);
		if(r != rp->rune)
			print(", chartorune => %C not %C", r, rp->rune);

		n = runetochar(buf, &rp->rune);
		if (n != utflen)
			print(", runetochar => %d for %d bytes", n, utflen);
		if (strcmp(buf, rp->utf) != 0)
			print(", runetochar => %s not %s", buf, rp->utf);

		print("\n");
	}
	exits(0);
}
