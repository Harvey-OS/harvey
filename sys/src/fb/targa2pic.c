#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>

#include "tga.h"

TGAFile	PicHdr;

/*
 * TGA files are always the PC's endian.  readshort will
 * read these files correctly regardless of the endian-ness of the local
 * machine.
 */
void
readshort(int fd, ushort *v) {
	uchar v1, v2;
	read(fd, &v2, 1);
	read(fd, &v1, 1);
	*v = (v1<<8) + v2;
}

char *
main(int argc, char *argv[]) {
	int	infd = 0;
	uchar **image;
	PICFILE *pf;
	int	BytesInPixel;
	int	BytesInRow;
	int	row;
	uchar *red, *green, *blue, *rowbuf;

	argc=getflags(argc, argv, "n:1[name]");
	if (argc == 1)
		infd = 0;
	else if (argc == 2)
		infd = open(argv[1], OREAD);
	else
		usage("[file]");
	if (infd < 0) {
		perror("opening targa file");
		exits("open");
	}
	
	read(infd, &PicHdr.idLength, 1);
	read(infd, &PicHdr.mapType, 1);
	read(infd, &PicHdr.imageType, 1);
	readshort(infd, &PicHdr.mapOrigin);
	readshort(infd, &PicHdr.mapLength);
	read(infd, &PicHdr.mapWidth, 1);
	readshort(infd, &PicHdr.xOrigin);
	readshort(infd, &PicHdr.yOrigin);
	readshort(infd, &PicHdr.imageWidth);
	readshort(infd, &PicHdr.imageHeight);
	read(infd, &PicHdr.pixelDepth, 1);
	read(infd, &PicHdr.imageDesc, 1);

	/*
	**	Read in the idString if its length != 0
	*/
	memset((void *)PicHdr.idString,0,256);
	if(PicHdr.idLength > 0)
		read(infd, PicHdr.idString, PicHdr.idLength);

	/* Reset to beginning of image data */

	seek(infd, (long)PicHdr.idLength + 18L, 0);
/*
	print("length	= %d\n", PicHdr.idLength);
	print("maptype	= %d\n", PicHdr.mapType);
	print("imagetype= %d\n", PicHdr.imageType);
	print("maporigin= %d\n", PicHdr.mapOrigin);
	print("maplength= %d\n", PicHdr.mapLength);
	print("width	= %d\n", PicHdr.mapWidth);
	print("x	= %d\n", PicHdr.xOrigin);
	print("y	= %d\n", PicHdr.yOrigin);
	print("width	= %d\n", PicHdr.imageWidth);
	print("height	= %d\n", PicHdr.imageHeight);
	print("depth	= %d\n", PicHdr.pixelDepth);
	print("desc	= %d\n", PicHdr.imageDesc);
/**/
	if((PicHdr.mapType != 0)) {
		fprint(2, "targa2pic: mapped files unimplemented\n");
		exits("mapped");
	}

	switch (PicHdr.imageType) {
	case TGAColorMapped:
	case TGARLEColorMapped:
		fprint(2, "targa2pic: color mapped files unimplemented\n");
		exits("mapped file");
	case TGARLETrueColor:
	case TGARLEBlackWhite:
		fprint(2, "targa2pic: compressed targa files unimplemented\n");
		exits("compressed");
	case TGATrueColor:
	case TGABlackWhite:
		break;
	default:
		fprint(2, "targa2pic: unknown image type %d\n", PicHdr.imageType);
		exits("unknown image type");
	}
	switch(PicHdr.pixelDepth){
	case 16:
	case 24:
	case 32:
		break;
	default:
		fprint(2, "targa2pic: Pixel depth not supported: %d\n",
			PicHdr.pixelDepth);
		exits("pixel depth");
	}

	BytesInPixel = PicHdr.pixelDepth/8;
	BytesInRow = BytesInPixel * PicHdr.imageWidth;

	/*
	 * check limitations
	 */
	pf = picopen_w("/fd/1", "runcode", 0, 0, PicHdr.imageWidth,
		PicHdr.imageHeight, "rgb", argv, 0);
	if (pf == 0) {
		perror("targa2pic: could not open file");
		exits("open write");
	}

	if(flag['n']) picputprop(pf, "NAME", flag['n'][0]);
	else if(argc>1) picputprop(pf, "NAME", argv[1]);

	image = calloc(PicHdr.imageHeight, sizeof(uchar*));
	for (row = 0; row < PicHdr.imageHeight; row ++ ){
		image[PicHdr.imageHeight-row-1] = malloc(BytesInRow);
		read(infd, &image[PicHdr.imageHeight-row-1][0], BytesInRow);
	}

	red = malloc(PicHdr.imageWidth);
	green = malloc(PicHdr.imageWidth);
	blue = malloc(PicHdr.imageWidth);
	rowbuf = malloc(3*PicHdr.imageWidth);
	for (row = 0; row < PicHdr.imageHeight; row ++ ) {
		uchar *b = &image[row][0];
		int j;

		switch(PicHdr.pixelDepth){
		case 16:
			for (j=0; j<PicHdr.imageWidth; j++, b += 2) {
				ushort p = (b[1] << 8) | b[0];
				blue[j] =	((p)>>0 & 0x1f) << 3;
				green[j] =	((p)>>5 & 0x1f) << 3;
				red[j] =	((p)>>10 & 0x1f) << 3;
			}
			break;
		case 24:
			for (j=0; j<PicHdr.imageWidth; j++, b += 3) {
				blue[j] =	b[0];
				green[j] =	b[1];
				red[j] =	b[2];
			}
			break;
		case 32:
			for (j=0; j<PicHdr.imageWidth; j++, b += 4) {
				blue[j] =	b[0];
				green[j] =	b[1];
				red[j] =	b[2];
			}
			break;
		}
		picpack(pf, (char *)rowbuf, "ccc", red, green, blue);
		picwrite(pf, rowbuf);
	}
	picclose(pf);
	return 0;
}
