#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int	rev(int);

void
main(int argc, char *argv[])
{
	Biobuf *b, *f;
	int i, j, x, y, o;
	uchar *line;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	} ARGEND

	if(argc < 1)
		exits(0);


	f = Bopen(argv[0], OREAD);
	b = Bopen("/tmp/jepout.bm", OWRITE);

	o = 58;
	x = 128;
	y = (Bseek(f, 0, 2) - o) / 128;
	Bseek(f, o, 0);
print("y = %d\n", y);
	line = malloc(x);

	Bprint(b, "%11s%12d%12d%12d%12d\n", "k1", 0, 0, x*8, y);

	for(i=0; i<y; i++) {
		for(j=0; j<x; j++)
			line[j] = Bgetc(f);
		for(j=x-1; j>=0; j--)
			Bputc(b, rev(line[j]));
	}

//	Bterm(b);
//	Bterm(f);
	exits(0);
}

int
rev(int c)
{
	int i, o;

	o = 0;
	for(i=0; i<8; i++) {
		o <<= 1;
		if(c & 1)
			o |= 1;
		c >>= 1;
	}
	return o^0xff;
}
