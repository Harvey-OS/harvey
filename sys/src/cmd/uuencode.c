/*
 * uuencode [input] - encode input as lines of printable ascii
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

void	encode(Biobuf*, Biobuf*);
void	outdec(Biobuf*f, char*);

/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) (((c) & 077) + ' ')

void
main(int argc, char **argv)
{
	char *infile, *name;
	Biobuf *in, *out;

	if(argc > 1){
		infile = name = *++argv;
		--argc;
	}else {
		infile = "/fd/0";
		name = "from_uuencode";
	}

	if(argc != 1){
		fprint(2, "Usage: uuencode [input]\n");
		exits("usage");
	}
	in = Bopen(infile, OREAD);
	if(in == 0)
		sysfatal("open %s: %r", infile);
	out = Bopen("/fd/1", OWRITE);
	Bprint(out, "begin 0664 %s\n", name);
	encode(in, out);
	Bprint(out, "end\n");
	exits(0);
}

/*
 * copy from in to out, encoding as you go along.
 */
void
encode(Biobuf *in, Biobuf *out)
{
	int i, n;
	char buf[80];

	do{
		/* 1 (up to) 45 character line */
		n = Bread(in, buf, 45);
		Bputc(out, ENC(n));
		for(i=0; i<n; i += 3)
			outdec(out, &buf[i]);
		Bputc(out, '\n');
	} while (n > 0);
}

/*
 * output one group of 3 bytes, pointed at by p, on file f.
 */
void
outdec(Biobuf *f, char *ap)
{
	int c1, c2, c3, c4;
	uchar *p;

	p = (uchar *)ap;
	c1 =  p[0] >> 2;
	c2 = (p[0] << 4) & 060 | p[1] >> 4;
	c3 = (p[1] << 2) & 074 | p[2] >> 6;
	c4 =  p[2] & 077;
	Bputc(f, ENC(c1));
	Bputc(f, ENC(c2));
	Bputc(f, ENC(c3));
	Bputc(f, ENC(c4));
}
