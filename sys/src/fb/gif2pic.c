/*
 * gif2pic.c  -  based strongly on gif2ras.c, whose author requires
 * me to reproduce the following:
 *
 * gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun Raster image.
 *
 * Copyright (c) 1988, 1989 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 */
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <libg.h>
#include <fb.h>
typedef unsigned char byte;
typedef int boolean;
#define NEXTBYTE (*ptr++)
#define IMAGESEP 0x2c
#define INTERLACEMASK 0x40
#define COLORMAPMASK 0x80
char *cmd;
int numcols;
FILE *fp;
int BitOffset = 0,			/* Bit Offset of next code */
	XC = 0, YC = 0,			/* Output X and Y coords of current pixel */
	Pass = 0,			/* Used by output routine if interlaced pic */
	OutCount = 0,			/* Decompressor output 'stack count' */
	RWidth, RHeight,		/* screen dimensions */
	Width, Height,			/* image dimensions */
	LeftOfs, TopOfs,		/* image offset */
	BitsPerPixel,			/* Bits per pixel, read from GIF header */
	BytesPerScanline,		/* bytes per scanline in output raster */
	ColorMapSize,			/* number of colors */
	Background,			/* background color */
	CodeSize,			/* Code size, read from GIF header */
	InitCodeSize,			/* Starting code size, used during Clear */
	Code,				/* Value returned by ReadCode */
	MaxCode,			/* limiting value for current code size */
	ClearCode,			/* GIF clear code */
	EOFCode,			/* GIF end-of-information code */
	CurCode, OldCode, InCode,	/* Decompressor variables */
	FirstFree,			/* First free code, generated per GIF spec */
	FreeCode,			/* Decompressor, next free slot in hash table */
	FinChar,			/* Decompressor variable */
	BitMask,			/* AND mask for data size */
	ReadMask;			/* Code AND mask for current code size */
boolean Interlace, HasColormap;
byte *Image;				/* The result array */
byte *RawGIF;				/* The heap array to hold it, raw */
byte *Raster;				/* The raster data stream, unblocked */
/* The hash table used by the decompressor */
int Prefix[4096];
int Suffix[4096];
/* An output array used by the decompressor */
int OutCode[1025];
/* The color map, read from the GIF header */
byte cmap[256][3];
char *id = "GIF87a";
void FatalError(char *s){
	fprintf(stderr, "%s: %s\n", cmd, s);
	exits("error");
}
/* Fetch the next code from the raster data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location in the Raster array as a BIT Offset.  We compute
 * the byte Offset into the raster array by dividing this by 8, pick up
 * three bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it. 
 */
int ReadCode(void){
	int RawCode, ByteOffset;
	ByteOffset = BitOffset / 8;
	RawCode = Raster[ByteOffset] + (0x100 * Raster[ByteOffset + 1]);
	if (CodeSize >= 8)
		RawCode += (0x10000 * Raster[ByteOffset + 2]);
	RawCode >>= (BitOffset % 8);
	BitOffset += CodeSize;
	return(RawCode & ReadMask);
}
void AddToPixel(byte Index){
	if (YC<Height)
		*(Image + YC * BytesPerScanline + XC) = Index;
	/* Update the X-coordinate, and if it overflows, update the Y-coordinate */
	if (++XC == Width) {
		/* If a non-interlaced picture, just increment YC to the next scan line. 
		 * If it's interlaced, deal with the interlace as described in the GIF
		 * spec.  Put the decoded scan line out to the screen if we haven't gone
		 * past the bottom of it
		 */
		XC = 0;
		if (!Interlace) YC++;
		else {
			switch (Pass) {
			case 0:
				YC += 8;
				if (YC >= Height) {
					Pass++;
					YC = 4;
				}
				break;
			case 1:
				YC += 8;
				if (YC >= Height) {
					Pass++;
					YC = 2;
				}
				break;
			case 2:
				YC += 4;
				if (YC >= Height) {
					Pass++;
					YC = 1;
				}
				break;
			case 3:
				YC += 2;
				break;
			}
		}
	}
}
void LoadGIF(char *fname){
	int filesize;
	byte ch, ch1;
	byte *ptr, *ptr1;
	int i;
	fp = fopen(fname,"r");
	if (!fp) FatalError("file not found");
	/* find the size of the file */
	fseek(fp, 0L, 2);
	filesize = ftell(fp);
	fseek(fp, 0L, 0);
	if (!(ptr = RawGIF = (byte *) malloc(filesize)))
		FatalError("not enough memory to read gif file");
	if (!(Raster = (byte *) malloc(filesize)))
		FatalError("not enough memory to read gif file");
	if (fread((char *)ptr, filesize, 1, fp) != 1)
		FatalError("GIF data read failed");
	if (strncmp((char *)ptr, id, 6))
		FatalError("not a GIF file");
	ptr += 6;
	/* Get variables from the GIF screen descriptor */
	ch = NEXTBYTE;
	RWidth = ch + 0x100 * NEXTBYTE;	/* screen dimensions... not used. */
	ch = NEXTBYTE;
	RHeight = ch + 0x100 * NEXTBYTE;
	ch = NEXTBYTE;
	HasColormap = ((ch & COLORMAPMASK) ? 1 : 0);
	BitsPerPixel = (ch & 7) + 1;
	numcols = ColorMapSize = 1 << BitsPerPixel;
	BitMask = ColorMapSize - 1;
	Background = NEXTBYTE;		/* background color... not used. */
	if (NEXTBYTE)		/* supposed to be NULL */
		FatalError("corrupt GIF file (bad screen descriptor)");
	/* Read in global colormap. */
	if (HasColormap) {
		for (i = 0; i < ColorMapSize; i++) {
			cmap[i][0] = NEXTBYTE;
			cmap[i][1] = NEXTBYTE;
			cmap[i][2] = NEXTBYTE;
		}
	}
	/* Check for image seperator */
	if (NEXTBYTE != IMAGESEP)
		FatalError("corrupt GIF file (no image separator)");
	/* Now read in values from the image descriptor */
	ch = NEXTBYTE;
	LeftOfs = ch + 0x100 * NEXTBYTE;
	ch = NEXTBYTE;
	TopOfs = ch + 0x100 * NEXTBYTE;
	ch = NEXTBYTE;
	Width = ch + 0x100 * NEXTBYTE;
	ch = NEXTBYTE;
	Height = ch + 0x100 * NEXTBYTE;
	Interlace = ((NEXTBYTE & INTERLACEMASK) ? 1 : 0);
	/* Note that I ignore the possible existence of a local color map.
	 * I'm told there aren't many files around that use them, and the spec
	 * says it's defined for future use.  This could lead to an error
	 * reading some files. 
	 */
	/* Start reading the raster data. First we get the intial code size
	 * and compute decompressor constant values, based on this code size.
	 */
	CodeSize = NEXTBYTE;
	ClearCode = (1 << CodeSize);
	EOFCode = ClearCode + 1;
	FreeCode = FirstFree = ClearCode + 2;
	/* The GIF spec has it that the code size is the code size used to
	 * compute the above values is the code size given in the file, but the
	 * code size used in compression/decompression is the code size given in
	 * the file plus one. (thus the ++).
	 */
	CodeSize++;
	InitCodeSize = CodeSize;
	MaxCode = (1 << CodeSize);
	ReadMask = MaxCode - 1;
	/* Read the raster data.  Here we just transpose it from the GIF array
	 * to the Raster array, turning it from a series of blocks into one long
	 * data stream, which makes life much easier for ReadCode().
	 */
	ptr1 = Raster;
	do {
		ch = ch1 = NEXTBYTE;
		while (ch--) *ptr1++ = NEXTBYTE;
		if ((Raster - ptr1) > filesize)
			FatalError("corrupt GIF file (unblock)");
	} while(ch1);
	free((char *)RawGIF);		/* We're done with the raw data now... */
	/* Allocate the X Image */
	Image = (byte *) malloc(Width*Height);
	if (!Image) FatalError("not enough memory for XImage");
	BytesPerScanline = Width;
	/* Decompress the file, continuing until you see the GIF EOF code.
	 * One obvious enhancement is to add checking for corrupt files here.
	 */
	Code = ReadCode();
	while (Code != EOFCode) {
		/* Clear code sets everything back to its initial value, then reads the
		 * immediately subsequent code as uncompressed data.
		 */
		if (Code == ClearCode) {
			CodeSize = InitCodeSize;
			MaxCode = (1 << CodeSize);
			ReadMask = MaxCode - 1;
			FreeCode = FirstFree;
			CurCode = OldCode = Code = ReadCode();
			FinChar = CurCode & BitMask;
			AddToPixel(FinChar);
		}
		else {
			/* If not a clear code, then must be data: save same as CurCode and InCode */
			CurCode = InCode = Code;
			/* If greater or equal to FreeCode, not in the hash table yet;
			 * repeat the last character decoded
			 */
			if (CurCode >= FreeCode) {
				CurCode = OldCode;
				OutCode[OutCount++] = FinChar;
			}
			/* Unless this code is raw data, pursue the chain pointed to by CurCode
			 * through the hash table to its end; each code in the chain puts its
			 * associated output code on the output queue.
			 */
			while (CurCode > BitMask) {
				if (OutCount > 1024)
					FatalError("Corrupt GIF file (OutCount)!\n");
				OutCode[OutCount++] = Suffix[CurCode];
				CurCode = Prefix[CurCode];
			}
			/* The last code in the chain is treated as raw data. */
			FinChar = CurCode & BitMask;
			OutCode[OutCount++] = FinChar;
			/* Now we put the data out to the Output routine.
			 * It's been stacked LIFO, so deal with it that way...
			 */
			for (i = OutCount - 1; i >= 0; i--)
				AddToPixel(OutCode[i]);
			OutCount = 0;
			/* Build the hash table on-the-fly. No table is stored in the file. */
			Prefix[FreeCode] = OldCode;
			Suffix[FreeCode] = FinChar;
			OldCode = InCode;
			/* Point to the next slot in the table.  If we exceed the current
			 * MaxCode value, increment the code size unless it's already 12.  If it
			 * is, do nothing: the next code decompressed better be CLEAR
			 */
			FreeCode++;
			if (FreeCode >= MaxCode) {
				if (CodeSize < 12) {
					CodeSize++;
					MaxCode *= 2;
					ReadMask = (1 << CodeSize) - 1;
				}
			}
		}
		Code = ReadCode();
	}
	free((char *)Raster);
	fclose(fp);
}
main(int argc, char *argv[]){
	PICFILE *p;
	int y;
	if(argc!=2){
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exits("usage");
	}
	cmd=argv[0];
	LoadGIF(argv[1]);
	p=picopen_w("OUT", "runcode", LeftOfs, TopOfs, BytesPerScanline, Height,
		"m", argv, HasColormap?(char *)cmap:0);
	if(p==0){
		picerror(argv[0]);
		exits("can't create OUT");
	}
	for(y=0;y!=Height;y++)
		picwrite(p, (char *)(Image+y*BytesPerScanline));
	exits(""); 
}
