#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<bio.h>
#include	"hdr.h"

/*
	produces the bitmap for the designated japanese characters
*/

static void usage(void);

void
main(int argc, char **argv)
{
	int from, to;
	int size = 24;
	char *file = "../han/jis.bits";
	long nc, nb;
	int x;
	uchar *bits;
	long *chars;
	int rawkut = 0;
	Bitmap *b, *b1;
	Subfont *f;
	Font *ff;
	int *found;

	ARGBEGIN{
	case 'k':	rawkut = 1; break;
	case 's':	size = 16; file = "../han/jis16.bits"; break;
	default:	usage();
	}ARGEND
	if(argc != 2)
		usage();
	from = strtol(argv[0], (char **)0, 0);
	to = strtol(argv[1], (char **)0, 0);
	binit(0, 0, "fontgen");
	nc = to-from+1;
	nb = size*size/8;		/* bytes per char */
	nb *= nc;
	bits = (uchar *)malloc(nb);
	chars = (long *)malloc(sizeof(long)*nc);
	found = (int *)malloc(sizeof(found[0])*nc);
	if(bits == 0 || chars == 0){
		fprint(2, "%s: couldn't malloc %d bytes for %d chars\n", argv0, nb, nc);
		exits("out of memory");
	}
	if(rawkut){
		for(x = from; x <= to; x++)
			chars[x-from] = x;
	} else
		map(from, to, chars);
	memset(bits, 0, nb);
	b = readbits(file, nc, chars, size, bits, &found);
	b1 = balloc(b->r, b->ldepth);
	bitblt(b1, b1->r.min, b, b->r, S);
/*	bitblt(&screen, add(screen.r.min, (Point){50,150}), b, b->r, S);
/**/
	f = bf(nc, size, b1, found);
/*	ff = mkfont(f, 0);
	string(&screen, add(screen.r.min, (Point){50,50}), ff, ">\001\002\122\123<", S);
/**/
	wrbitmapfile(1, b);
	wrsubfontfile(1, f);/**/
	exits(0);
}

static void
usage(void)
{
	fprint(2, "Usage: %s [-s] from to\n", argv0);
	exits("usage");
}
