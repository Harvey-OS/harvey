#include <u.h>
#include <libc.h>
#include <bio.h>

#define ESC	033
#define DEL	0177

uchar roman8[] = {
	/*       ¡     ¢     £     ¤     ¥     ¦     § */
	0xa0, 0xb8, 0xbf, 0xbb, 0xba, 0xbc, 0x20, 0xbd,

	/* ¨     ©     ª     «     ¬     ­     ®     ¯ */
	0xab, 0x20, 0xf9, 0xfb, 0x20, 0xf6, 0x20, 0xb0,

	/* °     ±     ²     ³     ´     µ     ¶     · */
	0xb3, 0xfe, 0x20, 0x20, 0xa8, 0x20, 0x20, 0xfc,

	/* ¸     ¹     º     »     ¼     ½     ¾     ¿ */
	0x20, 0x20, 0xfa, 0xfd, 0xf7, 0xf8, 0x20, 0xb9,

	/* À     Á     Â     Ã     Ä     Å     Æ     Ç */
	0xa1, 0xe0, 0xa2, 0xe1, 0xd8, 0xd0, 0xd3, 0xb4,

	/* È     É     Ê     Ë     Ì     Í     Î     Ï */
	0xa3, 0xdc, 0xa4, 0xa5, 0xe6, 0xe5, 0xa6, 0xa7,

	/* Ð     Ñ     Ò     Ó     Ô     Õ     Ö     × */
	0xe3, 0xb6, 0xe8, 0xe7, 0xdf, 0xe9, 0xda, 0x20,

	/* Ø     Ù     Ú     Û     Ü     Ý     Þ     ß */
	0xd2, 0xad, 0xed, 0xae, 0xdb, 0x20, 0xf1, 0xde,

	/* à     á     â     ã     ä     å     æ     ç */
	0xc8, 0xc4, 0xc0, 0xe2, 0xcc, 0xd4, 0xd7, 0xb5,

	/* è     é     ê     ë     ì     í     î     ï */
	0xc9, 0xc5, 0xc1, 0xcd, 0xd9, 0xd5, 0xd1, 0xdd,

	/* ð     ñ     ò     ó     ô     õ     ö     ÷ */
	0xe4, 0xb7, 0xca, 0xc6, 0xc2, 0xea, 0xce, 0x20,

	/* ø     ù     ú     û     ü     ý     þ     ÿ */
	0xd6, 0xcb, 0xc7, 0xc3, 0xcf, 0x20, 0xf0, 0xef
};

void	think(void);

int	infd;
Biobuf *in;
int	rflag;
char *	oname = "/dev/lpt1data";
int	outfd;
Biobuf *out;
int	linepos;

void
main(int argc, char **argv)
{
	ARGBEGIN{
	case 'o':
		oname = ARGF();
		break;
	case 'r':
		++rflag;
		break;
	default:
		fprint(2, "usage: %s [-r] [-o file] [file...]\n", argv0);
		exits("usage");
	}ARGEND

	outfd = create(oname, OWRITE, 0664);
	if(outfd < 0){
		perror(oname);
		exits("create");
	}
	out = malloc(sizeof(Biobuf));
	Binit(out, outfd, OWRITE);
	in = malloc(sizeof(Biobuf));
	if(argc <= 0)
		think();
	else while(--argc >= 0){
		infd = open(*argv++, OREAD);
		if(infd < 0){
			perror(argv[-1]);
			continue;
		}
		think();
		close(infd);
	}
	exits(0);
}

void
think(void)
{
	int c, nspaces;

	Binit(in, infd, OREAD);
	while((c = Bgetrune(in)) >= 0)switch(c){	/* assign = */
	case '\b':
		BPUTC(out, '\b');
		if (linepos > 0)
			linepos--;
		break;
	case '\t':
		linepos += nspaces = 8 - linepos%8;
		Bwrite(out, "        ", nspaces);
		break;
	case '\n':
		BPUTC(out, '\n');
		/* fall through */
	case '\r':
		BPUTC(out, '\r');
		linepos = 0;
		break;
	case ESC:
		BPUTC(out, ESC);
		if((c = BGETC(in)) < 0)	/* assign = */
			break;
		BPUTC(out, c);
		if(c == 'z' || c == '=')
			break;
		while((c < '@' || c > 'Z') && (c = BGETC(in)) >= 0)
			BPUTC(out, c);
		break;
	default:
		if(c >= 0xa0 && !rflag)
			c = roman8[c - 0xa0];
		BPUTC(out, c);
		if((c &= 0x7f) >= ' ' && c < DEL)
			++linepos;
	}
}
