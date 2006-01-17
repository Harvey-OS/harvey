/* Copyright (C) 1996, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevccr.c,v 1.6 2005/01/19 00:24:07 dan Exp $*/
/* CalComp Raster Format driver */
#include "gdevprn.h"

/*
 * Please contact the author, Ernst Muellner (ernst.muellner@oenzl.siemens.de),
 * if you have any questions about this driver.
 */

#define CCFILESTART(p) putc(0x02, p)
#define CCFILEEND(p) putc(0x04, p)
#define CCNEWPASS(p) putc(0x0c, p)
#define CCEMPTYLINE(p) putc(0x0a, p)
#define CCLINESTART(len,p) do{ putc(0x1b,p);putc(0x4b,p);putc(len>>8,p); \
			       putc(len&0xff,p);} while(0)

#define CPASS (0)
#define MPASS (1)
#define YPASS (2)
#define NPASS (3)


typedef struct cmyrow_s
	  {
	    int current;
            int _cmylen[NPASS];
	    int is_used;
	    char cname[4];
	    char mname[4];
	    char yname[4];
            unsigned char *_cmybuf[NPASS];
	  } cmyrow;

#define clen _cmylen[CPASS]
#define mlen _cmylen[MPASS]
#define ylen _cmylen[YPASS]
#define cmylen _cmylen

#define cbuf _cmybuf[CPASS]
#define mbuf _cmybuf[MPASS]
#define ybuf _cmybuf[YPASS]
#define cmybuf _cmybuf

private int alloc_rb( gs_memory_t *mem, cmyrow **rb, int rows);
private int alloc_line( gs_memory_t *mem, cmyrow *row, int cols);
private void add_cmy8(cmyrow *rb, char c, char m, char y);
private void write_cpass(cmyrow *buf, int rows, int pass, FILE * pstream);
private void free_rb_line( gs_memory_t *mem, cmyrow *rbuf, int rows, int cols);

struct gx_device_ccr_s {
	gx_device_common;
	gx_prn_device_common;
        /* optional parameters */
};
typedef struct gx_device_ccr_s gx_device_ccr;

#define bdev ((gx_device_ccr *)pdev)

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 300
#define Y_DPI 300
#define DEFAULT_WIDTH_10THS_A3 117
#define DEFAULT_HEIGHT_10THS_A3 165

/* Macro for generating ccr device descriptors. */
#define ccr_prn_device(procs, dev_name, margin, num_comp, depth, max_gray, max_rgb, print_page)\
{	prn_device_body(gx_device_ccr, procs, dev_name,\
	  DEFAULT_WIDTH_10THS_A3, DEFAULT_HEIGHT_10THS_A3, X_DPI, Y_DPI,\
	  margin, margin, margin, margin,\
	  num_comp, depth, max_gray, max_rgb, max_gray + 1, max_rgb + 1,\
	  print_page)\
}

/* For CCR, we need our own color mapping procedures. */
private dev_proc_map_rgb_color(ccr_map_rgb_color);
private dev_proc_map_color_rgb(ccr_map_color_rgb);


/* And of course we need our own print-page routine. */
private dev_proc_print_page(ccr_print_page);

/* The device procedures */
private gx_device_procs ccr_procs =
    prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		    ccr_map_rgb_color, ccr_map_color_rgb);

/* The device descriptors themselves */
gx_device_ccr far_data gs_ccr_device =
  ccr_prn_device(ccr_procs, "ccr", 0.2, 3, 8, 1, 1,
		 ccr_print_page);

/* ------ Color mapping routines ------ */
/* map an rgb color to a ccr cmy bitmap */
private gx_color_index
ccr_map_rgb_color(gx_device *pdev, const ushort cv[])
{
  ushort r, g, b;
  register int shift = gx_color_value_bits - 1;

  r = cv[0]; g = cv[1]; b = cv[2];
  r>>=shift;
  g>>=shift;
  b>>=shift;

  r=1-r; g=1-g; b=1-b; /* rgb -> cmy */
  return r<<2 | g<<1 | b;
}

/* map an ccr cmy bitmap to a rgb color */ 
private int
ccr_map_color_rgb(gx_device *pdev, gx_color_index color, ushort rgb[3])
{
  rgb[2]=(1-(color >>2))*gx_max_color_value; /* r */
  rgb[1]=(1-( (color & 0x2) >> 1))*gx_max_color_value; /* g */
  rgb[0]=(1-(color & 0x1))*gx_max_color_value; /* b */
  return 0;
}
/* ------ print page routine ------ */


private int
ccr_print_page(gx_device_printer *pdev, FILE *pstream)
{
  cmyrow *linebuf;
  int line_size = gdev_prn_raster((gx_device *)pdev);
  int pixnum = pdev->width;
  int lnum = pdev->height;
  int l, p, b;
  int cmy, c, m, y;
  byte *in;
  byte *data;

  if((in = (byte *)gs_malloc(pdev->memory, line_size, 1, "gsline")) == NULL)
     return_error(gs_error_VMerror);
    
  if(alloc_rb( pdev->memory, &linebuf, lnum))
    {
      gs_free(pdev->memory, in, line_size, 1, "gsline");
      return_error(gs_error_VMerror);
    }

  for ( l = 0; l < lnum; l++ )
     {	gdev_prn_get_bits(pdev, l, in, &data);
        if(alloc_line(pdev->memory, &linebuf[l], pixnum))
	  {
	    gs_free(pdev->memory, in, line_size, 1, "gsline");
	    free_rb_line( pdev->memory, linebuf, lnum, pixnum );
	    return_error(gs_error_VMerror);
	  }
        for ( p=0; p< pixnum; p+=8)
	  {
	    c=m=y=0;
            for(b=0; b<8; b++)
	    {
              c <<= 1; m <<= 1; y <<= 1;
	      if(p+b < pixnum)
		cmy = *data;
	      else
		cmy = 0;

              c |= cmy>>2;
	      m |= (cmy>>1) & 0x1;
	      y |= cmy & 0x1;
	      data++;
	    }
	    add_cmy8(&linebuf[l], c, m, y);
	  }
      }
CCFILESTART(pstream);
write_cpass(linebuf, lnum, YPASS, pstream);
CCNEWPASS(pstream);
write_cpass(linebuf, lnum, MPASS, pstream);
CCNEWPASS(pstream);
write_cpass(linebuf, lnum, CPASS, pstream);
CCFILEEND(pstream);		 

/* clean up */	      
gs_free(pdev->memory, in, line_size, 1, "gsline");
free_rb_line( pdev->memory, linebuf, lnum, pixnum );
return 0;
}


/* ------ Internal routines ------ */


private int alloc_rb( gs_memory_t *mem, cmyrow **rb, int rows)
  {
  *rb = (cmyrow*) gs_malloc(mem, rows, sizeof(cmyrow), "rb");
  if( *rb == 0)
    return_error(gs_error_VMerror);
  else
    {
      int r;
      for(r=0; r<rows; r++)
	{
	  sprintf((*rb)[r].cname, "C%02x", r);
	  sprintf((*rb)[r].mname, "M%02x", r);
	  sprintf((*rb)[r].yname, "Y%02x", r);
	  (*rb)[r].is_used=0;
	}
      return 0;
    }
}

private int alloc_line( gs_memory_t *mem, cmyrow *row, int cols)
{
  int suc;
  suc=((row->cbuf = (unsigned char *) gs_malloc(mem, cols,1, row->cname)) &&
       (row->mbuf = (unsigned char *) gs_malloc(mem, cols,1, row->mname)) &&
       (row->ybuf = (unsigned char *) gs_malloc(mem, cols,1, row->yname)));
  if(suc == 0)
       {
       gs_free(mem, row->cbuf, cols,1, row->cname);
       gs_free(mem, row->mbuf, cols,1, row->mname);
       gs_free(mem, row->ybuf, cols,1, row->yname);

       return_error(gs_error_VMerror);
     }
  row->is_used = 1;
  row->current = row->clen = row->mlen = row->ylen = 0;
  return 0;
}

private void add_cmy8(cmyrow *rb, char c, char m, char y)
{
  int cur=rb->current;
  rb->cbuf[cur]=c;
  if(c)
    rb->clen=cur+1;
  rb->mbuf[cur]=m;
  if(m)
    rb->mlen=cur+1;
  rb->ybuf[cur]=y;
  if(y)
    rb->ylen=cur+1;
  rb->current++;
  return;
}

private void write_cpass(cmyrow *buf, int rows, int pass, FILE * pstream)
{
  int row, len;
    for(row=0; row<rows; row++)
      {
      len=buf[row].cmylen[pass];
      if(len == 0)
	CCEMPTYLINE(pstream);
      else
	{
	  CCLINESTART(len,pstream);
          fwrite( buf[row].cmybuf[pass], len, 1, pstream);
        }
    }
  return;
}

private void free_rb_line( gs_memory_t *mem, cmyrow *rbuf, int rows, int cols)
{
  int i;
  for(i=0; i<rows; i++)
    {
      if(rbuf[i].is_used)
	{
          gs_free(mem, rbuf[i].cbuf, cols, 1, rbuf[i].cname);
	  gs_free(mem, rbuf[i].mbuf, cols, 1, rbuf[i].mname);
	  gs_free(mem, rbuf[i].ybuf, cols, 1, rbuf[i].yname);
	  rbuf[i].is_used = 0;
	}
      else
	break;
    }
  gs_free( mem, rbuf, rows, sizeof(cmyrow),  "rb");
  return;
}
