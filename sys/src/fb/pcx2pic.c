#include <u.h>
#include <libc.h>
#include <bio.h>

/*
 * 0	Manufacturer	1	Constant Flag  10 = ZSoft .PCX
 * 1	Version 	1	Version information:
 * 				0 = Version 2.5
 * 				2 = Version 2.8 w/palette information
 * 				3 = Version 2.8 w/o palette information
 * 				5 = Version 3.0
 * 2	Encoding	1	1 = .PCX run length encoding
 * 3	Bits per pixel	1	Number of bits/pixel per plane
 * 4	Window  	8	Picture Dimensions 
 * 				(Xmin, Ymin) - (Xmax - Ymax)
 * 				in pixels, inclusive
 * 12	HRes    	2	Horizontal Resolution of creating device
 * 14	VRes    	2	Vertical Resolution of creating device
 * 16	Colormap	48	Color palette setting, see text
 * 64	Reserved	1
 * 65	NPlanes	        1	Number of color planes
 * 66	Bytes per Line	2	Number of bytes per scan line per 
 * 				color plane (always even for .PCX files)
 * 68	Palette Info	2	How to interpret palette - 1 = color/BW,
 * 				2 = grayscale
 * 70	Filler  	58	blank to fill out 128 byte header
 */

typedef struct Pcxhdr	Pcxhdr;

struct Pcxhdr
{
	uchar	manufacturer;
	uchar	version;
	uchar	encoding;
	uchar	bits2pixel;
	short	xmin;
	short	ymin;
	short	xmax;
	short	ymax;
	short	hres;
	short	vres;
	uchar	colormap[48];
	uchar	reserved;
	uchar	nplanes;
	short	bytes2line;
	short	pallette;
	uchar	filler[58];
	uchar *	cmap;		/* might point to a long one */
};

Pcxhdr *Bpcxhdr(Biobuf*);
void	Bpcxhdump(Biobuf*, Pcxhdr*);
void	pcxcmap(Biobuf*, Pcxhdr*);

int	debug;
int	infd;
Biobuf *in;
Biobuf *out;

int	rflag;

void
main(int argc, char **argv)
{
	Pcxhdr *h;
	int c, i, j, k, n;
	char *name=0;

	ARGBEGIN{
	default:
		fprint(2, "Usage: %s [-rD] [-n name] [file]\n", argv0);
		exits("usage");
	case 'n':
		name=ARGF();
		break;
	case 'r':
		++rflag;
		break;
	case 'D':
		++debug;
		break;
	}ARGEND
	if(argc > 0){
		infd = open(argv[0], OREAD);
		if(infd < 0){
			perror(argv[0]);
			exits("open");
		}
	}
	in = malloc(sizeof(Biobuf));
	Binit(in, infd, OREAD);
	out = malloc(sizeof(Biobuf));
	Binit(out, 1, OWRITE);

	h = Bpcxhdr(in);
	if(debug)
		Bpcxhdump(out, h);
	if(h->bits2pixel == 1){
		rflag = 0;
		h->cmap = 0;
	}else
		pcxcmap(in, h);

	Bprint(out, "TYPE=%s\n", h->bits2pixel==1 ? "bitmap" :
		rflag ? "runcode" : "dump");
	Bprint(out, "WINDOW=%d %d %d %d\n", h->xmin, h->ymin, h->xmax+1, h->ymax+1);
	Bprint(out, "NCHAN=%d\n", h->nplanes);
	Bprint(out, "CHAN=m\n");
	Bprint(out, "RES=%d %d\n", h->hres, h->vres);
	if(name) Bprint(out, "NAME=%s", name);
	else if(argc>0) Bprint(out, "NAME=%s", argv[0]);
	if(h->cmap)
		Bprint(out, "CMAP=\n");
	Bprint(out, "\n");

	if(debug)
		exits(0);

	if(h->cmap)
		Bwrite(out, h->cmap, 3*256);

	for(i=h->ymin; i<=h->ymax; i++){
		for(j=0; j<=h->bytes2line; j+=n){
			c = BGETC(in);
			if(c < 0)
				goto Eof;
			if((c&0xc0) == 0xc0){
				n = c & 0x3f;
				c = BGETC(in);
				if(c < 0)
					goto Eof;
			}else
				n = 1;
			if(rflag){
				BPUTC(out, n-1);
				BPUTC(out, c);
			}else
				for(k=0; k<n; k++)
					BPUTC(out, c);
		}
	}
	exits(0);

Eof:
	fprint(2, "unexpected eof y=%d, j=%d\n", i, j);
	exits("eof");
}

void
pcxcmap(Biobuf *bp, Pcxhdr *h)
{
	int n, k;

	if(h->version != 5 || h->encoding != 1 || h->bits2pixel != 8 ||
		h->nplanes != 1 || h->pallette != 1){
		fprint(2, "unsupported format\n");
		exits("format");
	}
	k = Boffset(bp);
	Bseek(bp, -3*256-1, 2);
	n = Bgetc(bp);
	if(n != 12){
		fprint(2, "bad colormap %d\n", n);
		exits("colormap");
	}
	h->cmap = malloc(3*256);
	n = Bread(bp, h->cmap, 3*256);
	if(n != 3*256){
		fprint(2, "bad colormap, read %d, expected %d\n", n, 3*256);
		exits("colormap");
	}
	Bseek(bp, k, 0);
}

#define	CHAR(m)		(h->m = BGETC(bp))
#define	SHORT(m)	(h->m = BGETC(bp), h->m |= BGETC(bp)<<8)
#define	BYTES(m,n)	Bread(bp, h->m, n)
Pcxhdr *
Bpcxhdr(Biobuf *bp)
{
	Pcxhdr *h;

	h = malloc(sizeof(Pcxhdr));
	CHAR(manufacturer);
	CHAR(version);
	CHAR(encoding);
	CHAR(bits2pixel);
	SHORT(xmin);
	SHORT(ymin);
	SHORT(xmax);
	SHORT(ymax);
	SHORT(hres);
	SHORT(vres);
	BYTES(colormap, 48);
	CHAR(reserved);
	CHAR(nplanes);
	SHORT(bytes2line);
	SHORT(pallette);
	BYTES(filler, 58);
	h->cmap = h->colormap;
	return h;
}
#undef	CHAR
#undef	SHORT
#undef	BYTES

void
Bpcxhdump(Biobuf *bp, Pcxhdr *h)
{
	Bprint(bp, "manufacturer = %d\n", h->manufacturer);
	Bprint(bp, "version = %d\n", h->version);
	Bprint(bp, "encoding = %d\n", h->encoding);
	Bprint(bp, "bits2pixel = %d\n", h->bits2pixel);
	Bprint(bp, "xmin = %d\n", h->xmin);
	Bprint(bp, "ymin = %d\n", h->ymin);
	Bprint(bp, "xmax = %d\n", h->xmax);
	Bprint(bp, "ymax = %d\n", h->ymax);
	Bprint(bp, "hres = %d\n", h->hres);
	Bprint(bp, "vres = %d\n", h->vres);
	Bprint(bp, "nplanes = %d\n", h->nplanes);
	Bprint(bp, "bytes2line = %d\n", h->bytes2line);
	Bprint(bp, "pallette = %d\n", h->pallette);
}
