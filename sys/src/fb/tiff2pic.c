#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <stdio.h>
#include <stdarg.h>
#include <libtiff.h>
/*
 * exits defined here because of ludicrous library lossage
 */
void exits(char *s){
	void exit(int);
	exit(s && *s);
}
void explode(uchar *inbuf, char *outbuf, int npix, int skip, int nbit){
	uchar inp;
	int shift, mask, mul;
	switch(nbit){
	default:
		fprint(2, "Bad nbit!\n");
		exits("bad nbit");
	case 1: mask=0x01; mul=0xff; break;
	case 2: mask=0x03; mul=0x55; break;
	case 4: mask=0x0f; mul=0x11; break;
	case 8: mask=0xff; mul=0x01; break;
	}
	shift=-1;
	for(;npix!=0;--npix){
		if(shift<0){
			inp=*inbuf++;
			shift=8-nbit;
		}
		*outbuf=((inp>>shift)&mask)*mul;
		shift-=nbit;
		outbuf+=skip;
	}
}
void main(int argc, char *argv[]){
	TIFF *in;
	PICFILE *out;
	ulong x, y, nx, ny;
	ushort c, nchan, nbit, config;
	char *chan, *outbuf;
	uchar *inbuf;
	argc=getflags(argc, argv, "n:1[name]");
	if(argc!=2) usage("file");
	in=TIFFOpen(argv[1], "r");
	if(in==0){
		fprint(2, "%s: can't open input\n", argv[0]);
		exits("open input");
	}
	TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &nbit);
	TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &nchan);
	TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &nx);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &ny);
	TIFFGetField(in, TIFFTAG_PLANARCONFIG, &config);
	if(nbit!=1 && nbit!=2 && nbit!=4 && nbit!=8){
		fprint(2, "%s: sorry, can't do bits/sample=%d\n", argv[0], nbit);
		exits("weird byte");
	}
	switch(nchan){
	default:
		fprint(2, "%s: sorry, we don't support nchan=%d\n", argv[0], nchan);
		break;
	case 1: chan="m"; break;
	case 2: chan="ma"; break;
	case 3: chan="rgb"; break;
	case 4: chan="rgba"; break;
	case 5: chan="mz..."; break;
	case 6: chan="maz..."; break;
	case 7: chan="rgbz..."; break;
	case 8: chan="rgbaz..."; break;
	}
	out=picopen_w("OUT", "runcode", 0, 0, nx, ny, chan, argv, 0);
	picputprop(out, "NAME", flag['n']?flag['n'][0]:argv[1]);
	switch(config){	/* what an annoyance! */
	case PLANARCONFIG_CONTIG:
		inbuf=malloc(nx*nchan);
		outbuf=malloc(nx*nchan);
		if(inbuf==0 || outbuf==0){
		NoMem:
			fprint(2, "%s: can't malloc\n", argv[0]);
			exits("no mem");
		}
		for(y=0;y!=ny;y++){
			if(TIFFReadScanline(in, (unsigned char *)inbuf, y, 0)<0){
			BadRead:
				fprint(2, "%s: bad read\n", argv[0]);
				exits("bad read");
			}
			explode(inbuf, outbuf, nx*nchan, 1, nbit);
			picwrite(out, outbuf);
		}
		break;
	case PLANARCONFIG_SEPARATE:
		inbuf=malloc(nx);
		outbuf=malloc(nx*nchan);
		if(inbuf==0 || outbuf==0) goto NoMem;
		for(y=0;y!=ny;y++){
			for(c=0;c!=nchan;c++){
				if(TIFFReadScanline(in, inbuf, y, c)<0)
					goto BadRead;
				explode(inbuf, outbuf+c, nx, nchan, nbit);
			}
			picwrite(out, outbuf);
		}
	}
	exits("");
}
