/*
 * uudecode [input] - decode input produced by uuencode
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

void	decode(Biobuf*, Biobuf*);
void	outdec(Biobuf*, char*, int);

/* single character decode */
#define DEC(c)	(((c) - ' ') & 077)

void
main(int argc, char **argv)
{
	char *infile, *l;
	Biobuf *in, *out;
	int n, mode;

	/* optional input arg */
	if(argc > 1){
		infile = argv[1];
		argc--;
	}else
		infile = "/fd/0";

	if(argc != 1){
		fprint(2, "Usage: uudecode [infile]\n");
		exits("usage");
	}
	in = Bopen(infile, OREAD);
	if(in == 0)
		sysfatal("open %s: %r", infile);

	/* search for header line */
	for(;;){
		l = Brdline(in, '\n');
		if(l == 0)
			sysfatal("no begin line");
		n = Blinelen(in);
		if(n > 6 && strncmp(l, "begin ", 6) == 0)
			break;
	}
	l[n-1] = 0;
	l += 6;
	mode = strtol(l, &l, 8);
	USED(mode);
	l = strrchr(l, ' ');
	if(l == 0 || *++l == 0)
		sysfatal("bad begin line");

	fprint(2, "uudecode: original file named %s\n", l);

	/* create output file */
	out = Bopen("/fd/1", OWRITE);
	if(out == 0)
		sysfatal("open /fd/1: %r");

	decode(in, out);

	l = Brdline(in, '\n');
	n = Blinelen(in);
	if(l == 0 || n != 4 || strncmp(l, "end\n", 4) != 0)
		sysfatal("no end line");
	exits(0);
}

/*
 * copy from in to out, decoding as you go along.
 */
void
decode(Biobuf *in, Biobuf *out)
{
	char *bp;
	int n;

	for(;;){
		/* for each input line */
		bp = Brdline(in, '\n');
		if(bp == 0)
			sysfatal("short file");
		n = DEC(*bp++);
		if(n <= 0)
			break;
		for(; n > 0; n -= 3){
			outdec(out, bp, n);
			bp += 4;
		}
	}
}

/*
 * output a group of 3 bytes (4 input characters).
 * the input chars are pointed to by p, they are to
 * be output to file f.  n is used to tell us not to
 * output all of them at the end of the file.
 */
void
outdec(Biobuf *f, char *p, int n)
{
	int c1, c2, c3;

	c1 = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
	c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
	c3 = DEC(p[2]) << 6 | DEC(p[3]);
	if(n >= 1)
		Bputc(f, c1);
	if(n >= 2)
		Bputc(f, c2);
	if(n >= 3)
		Bputc(f, c3);
}
