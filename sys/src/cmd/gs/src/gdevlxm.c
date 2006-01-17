/* Copyright (C) 1989-1994, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gdevlxm.c,v 1.5 2004/08/10 13:02:36 stefan Exp $*/
/*
 * Lexmark 5700 ink-jet printer driver for Ghostscript
 *
 * defines the lxm5700m device for printing in black-and-white at 1200 dpi
 * doesn't handle color or any other resolution.
 * Native resolution appears to be 600 x 1200, but print bands are overlapped.
 *
 * I use the command
 * gs -sOutputFile=/dev/lp0 -sDevice=lxm5700m -dHeadSeparation=15 file.ps
 *
 * where HeadSeparation varies from print-cartridge to print-cartridge and
 * 16 (the default) usually works fine.
 *
 *   Stephen Taylor  setaylor@ma.ultranet.com  staylor@cs.wpi.edu
 */

#include "gdevprn.h"
#include "gsparams.h"

/* The procedure descriptors */
/* declare functions */
private dev_proc_print_page(lxm5700m_print_page);
private dev_proc_get_params(lxm_get_params);
private dev_proc_put_params(lxm_put_params);

/* set up dispatch table.  I follow gdevdjet in using gdev_prn_output_page */
static const gx_device_procs lxm5700m_procs = 
    prn_params_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
                     lxm_get_params, lxm_put_params);

/* The device descriptors */

/* define a subclass with useful state in it. */
typedef struct lxm_device_s { /* a sub-class of gx_device_printer */
    gx_device_common;
    gx_prn_device_common;
    int headSeparation;
} lxm_device;

/* Standard lxm5700m device */
lxm_device far_data gs_lxm5700m_device = {
    prn_device_std_body(lxm_device, lxm5700m_procs, "lxm5700m",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	600, 600,	/* x dpi, y dpi */
	0.2, 0.0, 0.0, 0.0,			/* margins */
	1, lxm5700m_print_page),
    16   /* default headSeparation value */
};

/* I don't know the whole protocol for the printer, but let me tell
 * you about the fraction I use.
 * Each page begins with a header, which I describe as init1, init2, init3 --
 *  (I separate them, because I've seen output in which those sections
 *   seemed to appear independently, but I've deleted the only code I
 *   had that actually used them separately.)
 * Then there are a number of swipe commands, each of which describes one
 * swipe of the printhead.  Each swipe command begins with a fixed
 * header, which gives the number of bytes in the command,
 * left and right margins,
 * a vertical offset, some noise characters I don't know the meaning of,
 * and the table of bits.  The bits are given one column at a time, using a
 * simple compression scheme: a directory, consisting of two bytes, tells
 * which sixteen-bit intervals of the 208-bit column contain 1-bits; then
 * the appropriate number of sixteen-bit bit-maps follow.  (A zero-bit in the
 * directory indicates that a bitmap for that sector follows.) In the worst case,
 * this scheme would be bigger than the uncompressed bitmap, but it seems to
 * usually save 80-90%.  The biggest complication of the bitmap scheme is this:
 * There are two print-heads on the black cartridge, and they are 16 pixels
 * (or headSeparation) apart.  Odd columns of the swipe address one printhead,
 * and even columns the other.  On the following swipe, the printheads
 * addressed by even and odd columns are reversed.  I think the printheads might be
 * staggered, but the output I've seen staggers things in software;
 * adjacent vertical bits on the same head are not addressed; the missing
 * bits are written in by the second head when it passes (16 columns later.)
 * In my code, I call the state of addressing one head or the other "direction".
 * Originally I thought that the printhead was writing on both leftward and
 * rightward motion, and that which head was addressed by odd columns changed
 * accordingly.  I'm no longer sure this is true, but the head addressed by the
 * even columns does alternate with each swipe.
 */
/*
 * various output shorthands
 */
 
#define init1() \
	top(), \
	0xA5,0, 3, 0x40,4,5, \
	0xA5,0, 3, 0x40,4,6, \
	0xA5,0, 3, 0x40,4,7, \
	0xA5,0, 3, 0x40,4,8, \
	0xA5,0, 4, 0x40,0xe0,0x0b, 3 

#define init2() \
	0xA5,0, 11, 0x40,0xe0,0x41, 0,0,0,0,0,0,0, 2, \
	0xA5,0, 6, 0x40, 5, 0,0,0x80,0 \

#define init3()  \
	0x1b,'*', 7,0x73,0x30, \
	0x1b,'*', 'm', 0, 0x14, 3, 0x84, 2, 0, 1, 0xf4, \
	0x1b,'*', 7,0x63, \
	0x1b,'*', 'm', 0, 0x42,  0, 0, \
	0xA5,0, 5, 0x40,0xe0,0x80, 8, 7, \
	0x1b,'*', 'm', 0, 0x40, 0x15, 7, 0x0f, 0x0f  \

#define top()  \
	0xA5,0, 6, 0x40, 3,3,0xc0,0x0f,0x0f \

#define fin()  \
	0x1b,'*', 7, 0x65 \


#define outByte(b) putc(b, prn_stream)

#define RIGHTWARD 0
#define LEFTWARD 1
/* number of pixels between even columns in output and odd ones*/
/* #define headSeparation 16 */
/* overlap between successive swipes of the print head */
#define overLap 104
/* height of printhead in pixels */
#define swipeHeight 208
/* number of shorts described by each column directory */
#define directorySize 13 

/* ------ Driver procedures ------ */


/* Send the page to the printer. */
private int
lxm5700m_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	
    int lnum,minX, maxX, i, l, highestX, leastX, extent;
    int direction = RIGHTWARD;
    int lastY = 0;
    
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
    /* Note that in_size is a multiple of 8. */
    int in_size = line_size * (swipeHeight);
    int swipeBuf_size = in_size;
    byte *buf1 = (byte *)gs_malloc(pdev->memory, in_size, 1, "lxm_print_page(buf1)");
    byte *swipeBuf =
	(byte *)gs_malloc(pdev->memory, swipeBuf_size, 1, "lxm_print_page(swipeBuf)");
    byte *in = buf1;

    /* Check allocations */
    if ( buf1 == 0 || swipeBuf == 0 ) {
	if ( buf1 ) 
quit_ignomiously: /* and a goto into an if statement is pretty ignomious! */
	gs_free(pdev->memory, (char *)buf1, in_size, 1, "lxm_print_page(buf1)");
	if ( swipeBuf ) 
	    gs_free(pdev->memory, (char *)swipeBuf, swipeBuf_size, 1, "lxm_print_page(swipeBuf)");
	return_error(gs_error_VMerror);
    }

    {	/* Initialize the printer and reset the margins. */
	static const char init_string[] = {
	    init1(),
	    init2(),
	    init3()
	};
	fwrite(init_string, 1, sizeof(init_string), prn_stream);
    }
    /* Print lines of graphics */
    for (lnum=0; lnum < pdev->height-swipeHeight ; ) { /* increment in body */
	byte *in_data;
	register byte *outp;
	int lcnt;

	{	/* test for blank scan lines.  We  maintain the */
	    /* loop invariant lnum <pdev->height, but modify lnum */
	    int l;

	    for (l=lnum; l<pdev->height; l++) {
		/* Copy 1 scan line and test for all zero. */
		gdev_prn_get_bits(pdev, l, in, &in_data);
		if ( in_data[0] != 0 ||
		     memcmp((char *)in_data, (char *)in_data + 1, line_size - 1)
		     ) {
		    break;
		}
	    }/* end for l */

	    /* now l is the next non-blank scan line */
	    if (l >= pdev->height) {/* if there are no more bits on this page */
		lnum = l;
		break;			/* end the loop and eject the page*/
	    }

	    /* leave room for following swipe to reinforce these bits */
	    if (l-lnum > overLap) lnum = l - overLap;

	    /* if the first non-blank near bottom of page */
	    if (lnum >=pdev->height - swipeHeight) {
		/* don't move the printhead over empty air*/
		lnum = pdev->height - swipeHeight;
	    }
	}


	/* Copy the the scan lines. */
	lcnt = gdev_prn_copy_scan_lines(pdev, lnum, in, in_size);
	if ( lcnt < swipeHeight ) {
	    /* Pad with lines of zeros. */
	    memset(in + lcnt * line_size, 0,
		   in_size - lcnt * line_size);
	}

	/* compute right and left margin for this swipe */
	minX = line_size;
	maxX = 0;
	for (l=0; l<swipeHeight; l++) {/* for each line of swipe */
	    for (i=0; i<minX; i++) {/* look for left-most non-zero byte*/
		if (in[l*line_size+i] !=0) {
		    minX = i;
		    break;
		}
	    }
	    for (i=line_size-1; i>=maxX; i--) {/* look for right-most */
		if (in[l*line_size+i] !=0) {
		    maxX = i;
		    break;
		}
	    }
	}
	minX = (minX&(-2)); /* truncate to even */
	maxX = (maxX+3)&-2; /* raise to even */

	highestX = maxX*8-1;
	leastX = minX*8;
	extent = highestX -leastX +1;
		
	outp = swipeBuf;

	/* macro, not fcn call.  Space penalty is modest, speed helps */
#define buffer_store(x) if(outp-swipeBuf>=swipeBuf_size) {\
	    gs_free(pdev->memory, (char *)swipeBuf, swipeBuf_size, 1, "lxm_print_page(swipeBuf)");\
	    swipeBuf_size*=2;\
	    swipeBuf = (byte *)gs_malloc(pdev->memory, swipeBuf_size, 1, "lxm_print_page(swipeBuf)");\
	    if (swipeBuf == 0) goto quit_ignomiously;\
	    break;}\
	else *outp++ = (x)

	    {/* work out the bytes to store for this swipe*/

		int sx, sxBy8, sxMask;
		int words[directorySize];
		bool f, sum;
		int retval=0;
		int j,c,y;
		int j1,c1;
		int i,b,x, directory ;

		/* want to set up pointers for (upto two) stripes covered by the output*/

		/* now for each column covered by output: */
		for (x=leastX; x<=highestX; x++) {
		    for (i=0; i<directorySize; i++) {
			words[i] = 0;
		    }
		    directory = 0x2000;	/* empty directory != 0 */

		    /* prime loops: make comparisons here */
		    switch (direction) {
		    case(RIGHTWARD):
			sx = (x&1)==1 ? x : x-(((lxm_device*)pdev)->headSeparation);
			j1 = (x&1);	/* even if x even, odd if x odd */
			break;
		    default:	/* shouldn't happen ... but compilation checks */
		    case(LEFTWARD):
			sx = (x&1)==0 ? x : x-((lxm_device*)pdev)->headSeparation;
			j1 = 1-(x&1);	/* odd if x even, even if x odd */
		    }
		    c1 = 0x8000 >> j1;

		    sxBy8 = sx/8;
		    sxMask = 0x80>>(sx%8);

		    /* loop through all the swipeHeight bits of this column */
		    for (i = 0, b=1, y= sxBy8+j1*line_size; i < directorySize; i++,b<<=1) {
			sum = false;
			for (j=j1,c=c1 /*,y=i*16*line_size+sxBy8*/; j<16; j+=2, y+=2*line_size, c>>=2) {
			    f = (in[y]&sxMask);
			    if (f) {
				words[i] |= c;
				sum |= f;
			    }
			}
			if (!sum) directory |=b;
		    }
		    retval+=2;
		    buffer_store(directory>>8); buffer_store(directory&0xff);
		    if (directory != 0x3fff) {
			for (i=0; i<directorySize; i++) {
			    if (words[i] !=0) {
				buffer_store(words[i]>>8) ; buffer_store(words[i]&0xff);
				retval += 2;
			    }
			}
		    }
		}
#undef buffer_store
	    }
	{/* now write out header, then buffered bits */
	    int leastY = lnum;

	    /* compute size of swipe, needed for header */
	    int sz = 0x1a + outp - swipeBuf;

	    /* put out header*/
	    int deltaY = 2*(leastY - lastY);  /* vert coordinates here are 1200 dpi */
	    lastY = leastY;
	    outByte(0x1b); outByte('*'); outByte(3); 
	    outByte(deltaY>>8); outByte(deltaY&0xff);
	    outByte(0x1b); outByte('*'); outByte(4); outByte(0); outByte(0);
	    outByte(sz>>8); outByte(sz&0xff); outByte(0); outByte(3); 
	    outByte(1); outByte(1); outByte(0x1a);
	    outByte(0); 
	    outByte(extent>>8); outByte(extent&0xff); 
	    outByte(leastX>>8); outByte(leastX&0xff);
	    outByte(highestX>>8); outByte(highestX&0xff);
	    outByte(0); outByte(0);
	    outByte(0x22); outByte(0x33); outByte(0x44); 
	    outByte(0x55); outByte(1);
	    /* put out bytes */
	    fwrite(swipeBuf,1,outp-swipeBuf,prn_stream);
	}
	    lnum += overLap;
	    direction ^= 1;
    }/* ends the loop for swipes of the print head.*/

    /* Eject the page and reinitialize the printer */
    {
	static const char bottom[] = {
	    fin() /*,  looks like I can get away with only this much ...
	    init1(),
	    init3(),
	    fin()   , 
	    top(),
	    fin()  */
	};
	fwrite(bottom, 1, sizeof(bottom), prn_stream);
    }
    fflush(prn_stream);

    gs_free(pdev->memory, (char *)swipeBuf, swipeBuf_size, 1, "lxm_print_page(swipeBuf)");
    gs_free(pdev->memory, (char *)buf1, in_size, 1, "lxm_print_page(buf1)");
    return 0;
}

/* 
 * There are a number of parameters which can differ between ink cartridges. 
 * The Windows driver asks you to recalibrate every time you load a new
 * cartridge.
 * most of the parameters adjusted there relate to color, and so this 
 * monotone driver doesn't need them.  However, the Lexmark 5700 black
 * cartridge has two columns of dots, separated by about 16 pixels.
 * This `head separation' distance can vary between cartridges, so
 * we provide a parameter to set it.  In my small experience I've not
 * set the corresponding parameter in windows to anything greater than 17
 * or smaller than 15, but it would seem that it can vary from 1 to 32,
 * based on the calibration choices offered.
 *
 * As I understand the rules laid out in gsparams.h,
 * lxm_get_params is supposed to return the current values of parameters
 * and lxm_put_params is supposed to set up values in the lxm_device
 * structure which can be used by the lxm5700m_print_page routine.
 * I've copied my routines from gdevcdj.c
 */

private int
lxm_get_params(gx_device *pdev, gs_param_list *plist)
{       
    lxm_device* const ldev = (lxm_device*)pdev;
    int code = gdev_prn_get_params(pdev, plist);

    if ( code < 0 ) return code;
    code = param_write_int(plist, 
			   "HeadSeparation",
			   (int *)&(ldev->headSeparation));
           
    return code;
}

/* put_params is supposed to check all the parameters before setting any. */
private int
lxm_put_params(gx_device *pdev, gs_param_list *plist)
{
    int ecode;
    lxm_device* const ldev = (lxm_device*)pdev;
    int trialHeadSeparation=ldev->headSeparation;
    int code = param_read_int(plist, "HeadSeparation", &trialHeadSeparation);

    if ( trialHeadSeparation < 1 || trialHeadSeparation > 32 )
	param_signal_error(plist, "HeadSeparation", gs_error_rangecheck);
    /* looks like param_signal_error is not expected to return */
    ecode = gdev_prn_put_params(pdev, plist);	/* call super class put_params */
    if ( code < 0 ) return code;
    if (ecode < 0) return ecode;

    /* looks like everything okay; go ahead and set headSeparation */
    ldev->headSeparation = trialHeadSeparation;
    if ( code == 1) return ecode; /* I guess this means there is no "HeadSeparation" parameter */
    return 0;
}
