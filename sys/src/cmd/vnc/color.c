#include "vnc.h"
#include "vncv.h"

enum {
	RGB12 = CHAN4(CIgnore, 4, CRed, 4, CGreen, 4, CBlue, 4),
	BGR12 = CHAN4(CIgnore, 4, CBlue, 4, CGreen, 4, CRed, 4),
	BGR8 = CHAN3(CBlue, 2, CGreen, 3, CRed, 3),
};

void (*cvtpixels)(uchar*, uchar*, int);

static void
chan2fmt(Pixfmt *fmt, ulong chan)
{
	ulong c, rc, shift;

	shift = 0;
	for(rc = chan; rc; rc >>=8){
		c = rc & 0xFF;
		switch(TYPE(c)){
		case CRed:
			fmt->red = (Colorfmt){(1<<NBITS(c))-1, shift};
			break;
		case CBlue:
			fmt->blue = (Colorfmt){(1<<NBITS(c))-1, shift};
			break;
		case CGreen:
			fmt->green = (Colorfmt){(1<<NBITS(c))-1, shift};
			break;
		}
		shift += NBITS(c);
	}
}

/*
 * convert 32-bit data to 24-bit data by skipping
 * the last of every four bytes.  we skip the last
 * because we keep the server in little endian mode.
 */
static void
cvt32to24(uchar *dst, uchar *src, int npixel)
{
	int i;

	for(i=0; i<npixel; i++){
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		src++;
	}
}

/*
 * convert RGB12 (x4r4g4b4) into CMAP8
 */
static uchar rgb12[16*16*16];
static void
mkrgbtab(void)
{
	int r, g, b;

	for(r=0; r<16; r++)
	for(g=0; g<16; g++)
	for(b=0; b<16; b++)
		rgb12[r*256+g*16+b] = rgb2cmap(r*0x11, g*0x11, b*0x11);
}

static void
cvtrgb12tocmap8(uchar *dst, uchar *src, int npixel)
{
	int i, s;

	for(i=0; i<npixel; i++){
		s = (src[0] | (src[1]<<8)) & 0xFFF;
		*dst++ = rgb12[s];
		src += 2;
	}
}

/*
 * convert BGR8 (b2g3r3, default VNC format) to CMAP8 
 * some bits are lost.
 */
static uchar bgr8[256];
static void
mkbgrtab(void)
{
	int i, r, g, b;

	for(i=0; i<256; i++){
		b = i>>6;
		b = (b<<6)|(b<<4)|(b<<2)|b;
		g = (i>>3) & 7;
		g = (g<<5)|(g<<2)|(g>>1);
		r = i & 7;
		r = (r<<5)|(r<<2)|(r>>1);
		bgr8[i] = rgb2cmap(r, g, b);
	}
}

static void
cvtbgr332tocmap8(uchar *dst, uchar *src, int npixel)
{
	uchar *ed;

	ed = dst+npixel;
	while(dst < ed)
		*dst++ = bgr8[*src++];
}

void
choosecolor(Vnc *v)
{
	int bpp, depth;
	ulong chan;

	bpp = screen->depth;
	if((bpp / 8) * 8 != bpp)
		sysfatal("screen not supported");

	depth = screen->depth;
	chan = screen->chan;

	if(bpp == 24){
		if(verbose)
			fprint(2, "24bit emulation using 32bpp\n");
		bpp = 32;
		cvtpixels = cvt32to24;
	}

	if(chan == CMAP8){
		if(bpp12){
			if(verbose)
				fprint(2, "8bit emulation using 12bpp\n");
			bpp = 16;
			depth = 12;
			chan = RGB12;
			cvtpixels = cvtrgb12tocmap8;
			mkrgbtab();
		}else{
			if(verbose)
				fprint(2, "8bit emulation using 6bpp\n");	/* 6: we throw away 1 r, g bit */
			bpp = 8;
			depth = 8;
			chan = BGR8;
			cvtpixels = cvtbgr332tocmap8;
			mkbgrtab();
		}
	}

	v->bpp = bpp;
	v->depth = depth;
	v->truecolor = 1;
	v->bigendian = 0;
	chan2fmt(v, chan);
	if(v->red.max == 0 || v->green.max == 0 || v->blue.max == 0)
		sysfatal("screen not supported");

	if(verbose)
		fprint(2, "%d bpp, %d depth, 0x%lx chan, %d truecolor, %d bigendian\n",
			v->bpp, v->depth, screen->chan, v->truecolor, v->bigendian);

	/* send information to server */
	vncwrchar(v, MPixFmt);
	vncwrchar(v, 0);	/* padding */
	vncwrshort(v, 0);
	vncwrpixfmt(v, &v->Pixfmt);
	vncflush(v);
}
