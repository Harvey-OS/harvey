#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <stdio.h>
#include <stdarg.h>
#include "libtiff.h"
/*
 * exits defined here because of ludicrous library lossage
 */
void exits(char *s){
	void exit(int);
	exit(s && *s);
}
void main(int argc, char *argv[]){
	TIFF *in;
	PICFILE *out;
	ulong x, y, nx, ny;
	ushort c, nchan, nbit, config;
	char *chan, *buf, *outbuf;
	uchar *inbuf;
	if(argc!=2){
		fprint(2, "Usage: %s file\n", argv[0]);
		exits("usage");
	}
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
	if(nbit!=8){
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
	switch(config){	/* what an annoyance! */
	case PLANARCONFIG_CONTIG:
		buf=malloc(nx*nchan);
		if(buf==0){
		NoMem:
			fprint(2, "%s: can't malloc\n", argv[0]);
			exits("no mem");
		}
		for(y=0;y!=ny;y++){
			if(TIFFReadScanline(in, (unsigned char *)buf, y, 0)<0){
			BadRead:
				fprint(2, "%s: bad read\n", argv[0]);
				exits("bad read");
			}
			picwrite(out, buf);
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
				for(x=0;x!=nx;x++) outbuf[nchan*x+c]=inbuf[x];
			}
			picwrite(out, outbuf);
		}
	}
	exits("");
}
