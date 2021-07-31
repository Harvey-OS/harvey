//  work in progress...  this version only good enough to read
//  non-interleaved, 24bit RGB images

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <flate.h>
#include <draw.h>
#include "imagefile.h"

int debug;

enum{  IDATSIZE=1000000,
	FilterNone =	0,
	PropertyBit =	1<<5,
};

typedef struct ZlibR{
	Biobuf *bi;
	uchar *buf;
	uchar *b;	// next byte to decompress
	uchar *e;	// past end of buf
} ZlibR;

typedef struct ZlibW{
	uchar *r, *g, *b; // Rawimage channels
	int chan;	// next channel to write
	int col;	// column index of current pixel
			// -1 = one-byte pseudo-column for filter spec
	int row;	// row index of current pixel
	int ncol, nrow;	// image width, height
} ZlibW;

static ulong *crctab;
static uchar PNGmagic[] = {137,80,78,71,13,10,26,10};
static char readerr[] = "ReadPNG: read error: %r";
static char memerr[] = "ReadPNG: malloc failed: %r";

static ulong
get4(uchar *a)
{
	return (a[0]<<24) | (a[1]<<16) | (a[2]<<8) | a[3];
}

static
void
pnginit(void)
{
	static int inited;

	if(inited)
		return;
	inited = 1;
	crctab = mkcrctab(0xedb88320);
	if(crctab == nil)
		sysfatal("mkcrctab error");
	inflateinit();
}

static
void*
pngmalloc(ulong n, int clear)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal(memerr);
	if(clear)
		memset(p, 0, n);
	return p;
}

static int
getchunk(Biobuf *b, char *type, uchar *d, int m)
{
	uchar buf[8];
	ulong crc = 0, crc2;
	int n, nr;

	if(Bread(b, buf, 8) != 8)
		return -1;
	n = get4(buf);
	memmove(type, buf+4, 4);
	type[4] = 0;
	if(n > m)
		sysfatal("getchunk needed %d, had %d", n, m);
	nr = Bread(b, d, n);
	if(nr != n)
		sysfatal("getchunk read %d, expected %d", nr, n);
	crc = blockcrc(crctab, crc, type, 4);
	crc = blockcrc(crctab, crc, d, n);
	if(Bread(b, buf, 4) != 4)
		sysfatal("getchunk tlr failed");
	crc2 = get4(buf);
	if(crc != crc2)
		sysfatal("getchunk crc failed");
	return m;
}

static int
zread(void *va)
{
	ZlibR *z = va;
	char type[5];
	int n;

	if(z->b >= z->e){
refill_buffer:
		z->b = z->buf;
		n = getchunk(z->bi, type, z->b, z->e - z->b);
		if(n < 0 || strcmp(type, "IEND") == 0)
			return -1;
		if(type[0] & PropertyBit)
			goto refill_buffer;  /* skip auxiliary chunks for now */
		if(strcmp(type,"IDAT") != 0)
			sysfatal("unrecognized mandatory chunk %s", type);
	}
	return *z->b++;
}

static int
zwrite(void *va, void *vb, int n)
{
	ZlibW *z = va;
	uchar *buf = vb;
	int i;
	for(i=0; i<n; i++){
		if(z->col == -1){
			// skip filter byte
			buf++;
			z->col++;
			continue;
		}
		switch(z->chan){
		case 0:
			*z->r++ = *buf++;
			z->chan = 1;
			break;
		case 1:
			*z->g++ = *buf++;
			z->chan = 2;
			break;
		case 2:
			*z->b++ = *buf++;
			z->chan = 0;
			z->col++;
			if(z->col == z->ncol){
				z->col = -1;
				z->row++;
				if((z->row >= z->nrow) && (i < n-1) )
					sysfatal("header said %d rows; data goes further", z->nrow);
			}
			break;
		}
	}
	return n;
}

static Rawimage*
readslave(Biobuf *b)
{
	ZlibR zr;
	ZlibW zw;
	Rawimage *image;
	char type[5];
	uchar *buf, *h;
	int k, n, nrow, ncol, err;

	buf = pngmalloc(IDATSIZE, 0);
	Bread(b, buf, sizeof PNGmagic);
	if(memcmp(PNGmagic, buf, sizeof PNGmagic) != 0)
		sysfatal("bad PNGmagic");

	n = getchunk(b, type, buf, IDATSIZE);
	if(n < 13 || strcmp(type,"IHDR") != 0)
		sysfatal("missing IHDR chunk");
	h = buf;
	ncol = get4(h);  h += 4;
	nrow = get4(h);  h += 4;
	if(debug)
		fprint(2, "readpng nrow=%d ncol=%d\n", nrow, ncol);
	if(*h++ != 8)
		sysfatal("only 24 bit per pixel supported for now [%d]", h[-1]);
	if(*h++ != 2)
		sysfatal("only rgb supported for now [%d]", h[-1]);
	if(*h++ != 0)
		sysfatal("only deflate supported for now [%d]", h[-1]);
	if(*h++ != FilterNone)
		sysfatal("only FilterNone supported for now [%d]", h[-1]);
	if(*h != 0)
		sysfatal("only non-interlaced supported for now [%d]", h[-1]);

	image = pngmalloc(sizeof(Rawimage), 1);
	image->r = Rect(0, 0, ncol, nrow);
	image->cmap = nil;
	image->cmaplen = 0;
	image->chanlen = ncol*nrow;
	image->fields = 0;
	image->gifflags = 0;
	image->gifdelay = 0;
	image->giftrindex = 0;
	image->chandesc = CRGB;
	image->nchans = 3;
	for(k=0; k<3; k++)
		image->chans[k] = pngmalloc(ncol*nrow, 0);
	zr.bi = b;
	zr.buf = buf;
	zr.b = zr.e = buf + IDATSIZE;
	zw.r = image->chans[0];
	zw.g = image->chans[1];
	zw.b = image->chans[2];
	zw.chan = 0;
	zw.col = -1;
	zw.row = 0;
	zw.ncol = ncol;
	zw.nrow = nrow;
	err = inflatezlib(&zw, zwrite, &zr, zread);
	if(err)
		sysfatal("inflatezlib %s\n", flateerr(err));
	free(buf);
	return image;
}

Rawimage**
Breadpng(Biobuf *b, int colorspace)
{
	Rawimage *r, **array;
	char buf[ERRMAX];

	buf[0] = '\0';
	if(colorspace != CRGB){
		errstr(buf, sizeof buf);	/* throw it away */
		werrstr("ReadPNG: unknown color space %d", colorspace);
		return nil;
	}
	pnginit();
	array = malloc(2*sizeof(*array));
	if(array==nil)
		return nil;
	errstr(buf, sizeof buf);	/* throw it away */
	r = readslave(b);
	array[0] = r;
	array[1] = nil;
	return array;
}

Rawimage**
readpng(int fd, int colorspace)
{
	Rawimage** a;
	Biobuf b;

	if(Binit(&b, fd, OREAD) < 0)
		return nil;
	a = Breadpng(&b, colorspace);
	Bterm(&b);
	return a;
}
